'use strict';

let assert = require('assert');

require('./v3-models.js');

let BuildbotSyncer = require('./buildbot-syncer').BuildbotSyncer;

class BuildbotTriggerable {
    constructor(config, remote, buildbotRemote, slaveInfo, logger)
    {
        this._name = config.triggerableName;
        assert(typeof(this._name) == 'string', 'triggerableName must be specified');

        this._lookbackCount = config.lookbackCount;
        assert(typeof(this._lookbackCount) == 'number' && this._lookbackCount > 0, 'lookbackCount must be a number greater than 0');

        this._remote = remote;

        this._slaveInfo = slaveInfo;
        assert(typeof(slaveInfo.name) == 'string', 'slave name must be specified');
        assert(typeof(slaveInfo.password) == 'string', 'slave password must be specified');

        this._syncers = BuildbotSyncer._loadConfig(buildbotRemote, config);
        this._logger = logger || {log: () => { }, error: () => { }};
    }

    name() { return this._name; }

    updateTriggerable()
    {
        const map = new Map;
        let repositoryGroups = [];
        for (const syncer of this._syncers) {
            for (const config of syncer.testConfigurations()) {
                const entry = {test: config.test.id(), platform: config.platform.id()};
                map.set(entry.test + '-' + entry.platform, entry);
            }
            // FIXME: Move BuildbotSyncer._loadConfig here and store repository groups directly.
            repositoryGroups = syncer.repositoryGroups();
        }
        return this._remote.postJSONWithStatus(`/api/update-triggerable/`, {
            'slaveName': this._slaveInfo.name,
            'slavePassword': this._slaveInfo.password,
            'triggerable': this._name,
            'configurations': Array.from(map.values()),
            'repositoryGroups': Object.keys(repositoryGroups).map((groupName) => {
                const group = repositoryGroups[groupName];
                return {
                    name: groupName,
                    description: group.description,
                    acceptsRoots: group.acceptsRoots,
                    repositories: group.repositoryList,
                };
            })});
    }

    syncOnce()
    {
        let syncerList = this._syncers;
        let buildReqeustsByGroup = new Map;

        this._logger.log(`Fetching build requests for ${this._name}...`);
        let validRequests;
        return BuildRequest.fetchForTriggerable(this._name).then((buildRequests) => {
            validRequests = this._validateRequests(buildRequests);
            buildReqeustsByGroup = BuildbotTriggerable._testGroupMapForBuildRequests(buildRequests);
            return this._pullBuildbotOnAllSyncers(buildReqeustsByGroup);
        }).then((updates) => {
            this._logger.log('Scheduling builds');
            const promistList = [];
            const testGroupList = Array.from(buildReqeustsByGroup.values()).sort(function (a, b) { return a.groupOrder - b.groupOrder; });
            for (const group of testGroupList) {
                const nextRequest = this._nextRequestInGroup(group, updates);
                if (!validRequests.has(nextRequest))
                    continue;
                const promise = this._scheduleRequestIfSlaveIsAvailable(nextRequest, group.syncer, group.slaveName);
                if (promise)
                    promistList.push(promise);
            }
            return Promise.all(promistList);
        }).then(() => {
            // Pull all buildbots for the second time since the previous step may have scheduled more builds.
            return this._pullBuildbotOnAllSyncers(buildReqeustsByGroup);
        }).then((updates) => {
            // FIXME: Add a new API that just updates the requests.
            return this._remote.postJSONWithStatus(`/api/build-requests/${this._name}`, {
                'slaveName': this._slaveInfo.name,
                'slavePassword': this._slaveInfo.password,
                'buildRequestUpdates': updates});
        });
    }

    _validateRequests(buildRequests)
    {
        const testPlatformPairs = {};
        const validatedRequests = new Set;
        for (let request of buildRequests) {
            if (!this._syncers.some((syncer) => syncer.matchesConfiguration(request))) {
                const key = request.platform().id + '-' + request.test().id();
                if (!(key in testPlatformPairs))
                    this._logger.error(`Build request ${request.id()} has no matching configuration: "${request.test().fullName()}" on "${request.platform().name()}".`);
                testPlatformPairs[key] = true;
                continue;
            }
            const triggerable = request.triggerable();
            if (!triggerable) {
                this._logger.error(`Build request ${request.id()} does not specify a valid triggerable`);
                continue;
            }
            assert(triggerable instanceof Triggerable, 'Must specify a valid triggerable');
            assert.equal(triggerable.name(), this._name, 'Must specify the triggerable of this syncer');
            const repositoryGroup = request.repositoryGroup();
            if (!repositoryGroup) {
                this._logger.error(`Build request ${request.id()} does not specify a repository group. Such a build request is no longer supported.`);
                continue;
            }
            const acceptedGroups = triggerable.repositoryGroups();
            if (!acceptedGroups.includes(repositoryGroup)) {
                const acceptedNames = acceptedGroups.map((group) => group.name()).join(', ');
                this._logger.error(`Build request ${request.id()} specifies ${repositoryGroup.name()} but triggerable ${this._name} only accepts ${acceptedNames}`);
                continue;
            }
            validatedRequests.add(request);
        }

        return validatedRequests;
    }

    _pullBuildbotOnAllSyncers(buildReqeustsByGroup)
    {
        let updates = {};
        let associatedRequests = new Set;
        return Promise.all(this._syncers.map((syncer) => {
            return syncer.pullBuildbot(this._lookbackCount).then((entryList) => {
                for (const entry of entryList) {
                    const request = BuildRequest.findById(entry.buildRequestId());
                    if (!request)
                        continue;
                    associatedRequests.add(request);

                    const info = buildReqeustsByGroup.get(request.testGroupId());
                    assert(!info.syncer || info.syncer == syncer);
                    info.syncer = syncer;
                    if (entry.slaveName()) {
                        assert(!info.slaveName || info.slaveName == entry.slaveName());
                        info.slaveName = entry.slaveName();
                    }

                    const newStatus = entry.buildRequestStatusIfUpdateIsNeeded(request);
                    if (newStatus) {
                        this._logger.log(`Updating the status of build request ${request.id()} from ${request.status()} to ${newStatus}`);
                        updates[entry.buildRequestId()] = {status: newStatus, url: entry.url()};
                    } else if (!request.statusUrl()) {
                        this._logger.log(`Setting the status URL of build request ${request.id()} to ${entry.url()}`);
                        updates[entry.buildRequestId()] = {status: request.status(), url: entry.url()};
                    }
                }
            });
        })).then(() => {
            for (const request of BuildRequest.all()) {
                if (request.hasStarted() && !request.hasFinished() && !associatedRequests.has(request)) {
                    this._logger.log(`Updating the status of build request ${request.id()} from ${request.status()} to failedIfNotCompleted`);
                    assert(!(request.id() in updates));
                    updates[request.id()] = {status: 'failedIfNotCompleted'};
                }
            }
        }).then(() => updates);
    }

    _nextRequestInGroup(groupInfo, pendingUpdates)
    {
        for (const request of groupInfo.requests) {
            if (request.isScheduled() || (request.id() in pendingUpdates && pendingUpdates[request.id()]['status'] == 'scheduled'))
                return null;
            if (request.isPending() && !(request.id() in pendingUpdates))
                return request;
        }
        return null;
    }

    _scheduleRequestIfSlaveIsAvailable(nextRequest, syncer, slaveName)
    {
        if (!nextRequest)
            return null;

        if (!!nextRequest.order()) {
            if (syncer)
                return this._scheduleRequestWithLog(syncer, nextRequest, slaveName);
            this._logger.error(`Could not identify the syncer for ${nextRequest.id()}.`);
        }

        for (const syncer of this._syncers) {
            const promise = this._scheduleRequestWithLog(syncer, nextRequest, null);
            if (promise)
                return promise;
        }
        return null;
    }

    _scheduleRequestWithLog(syncer, request, slaveName)
    {
        const promise = syncer.scheduleRequestInGroupIfAvailable(request, slaveName);
        if (!promise)
            return promise;
        this._logger.log(`Scheduling build request ${request.id()}${slaveName ? ' on ' + slaveName : ''} in ${syncer.builderName()}`);
        return promise;
    }

    static _testGroupMapForBuildRequests(buildRequests)
    {
        const map = new Map;
        let groupOrder = 0;
        for (let request of buildRequests) {
            let groupId = request.testGroupId();
            if (!map.has(groupId)) // Don't use real TestGroup objects to avoid executing postgres query in the server
                map.set(groupId, {id: groupId, groupOrder: groupOrder++, requests: [request], syncer: null, slaveName: null});
            else
                map.get(groupId).requests.push(request);
        }
        return map;
    }
}

if (typeof module != 'undefined')
    module.exports.BuildbotTriggerable = BuildbotTriggerable;
