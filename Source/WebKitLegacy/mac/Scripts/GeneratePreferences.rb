#!/usr/bin/env ruby
#
# Copyright (c) 2017, 2020 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require "fileutils"
require 'erb'
require 'optparse'
require 'yaml'

options = {
  :basePreferences => nil,
  :debugPreferences => nil,
  :experimentalPreferences => nil,
  :internalPreferences => nil,
  :outputDirectory => nil
}
optparse = OptionParser.new do |opts|
    opts.banner = "Usage: #{File.basename($0)} --input file"

    opts.separator ""

    opts.on("--base input", "file to generate preferences from") { |basePreferences| options[:basePreferences] = basePreferences }
    opts.on("--debug input", "file to generate debug preferences from") { |debugPreferences| options[:debugPreferences] = debugPreferences }
    opts.on("--experimental input", "file to generate experimental preferences from") { |experimentalPreferences| options[:experimentalPreferences] = experimentalPreferences }
    opts.on("--internal input", "file to generate internal preferences from") { |internalPreferences| options[:internalPreferences] = internalPreferences }
    opts.on("--outputDir output", "directory to generate file in") { |output| options[:outputDirectory] = output }
end

optparse.parse!

if !options[:basePreferences] || !options[:debugPreferences] || !options[:experimentalPreferences] || !options[:internalPreferences]
  puts optparse
  exit -1
end

if !options[:outputDirectory]
  options[:outputDirectory] = Dir.getwd
end

FileUtils.mkdir_p(options[:outputDirectory])

parsedBasePreferences = begin
  YAML.load_file(options[:basePreferences])
rescue ArgumentError => e
  puts "Could not parse input file: #{e.message}"
  exit(-1)
end

parsedDebugPreferences = begin
  YAML.load_file(options[:debugPreferences])
rescue ArgumentError => e
  puts "Could not parse input file: #{e.message}"
  exit(-1)
end

parsedExperimentalPreferences = begin
  YAML.load_file(options[:experimentalPreferences])
rescue ArgumentError => e
  puts "Could not parse input file: #{e.message}"
  exit(-1)
end

parsedInternalPreferences = begin
  YAML.load_file(options[:internalPreferences])
rescue ArgumentError => e
  puts "Could not parse input file: #{e.message}"
  exit(-1)
end


class Preference
  attr_accessor :name
  attr_accessor :type
  attr_accessor :defaultValue
  attr_accessor :humanReadableName
  attr_accessor :humanReadableDescription
  attr_accessor :webcoreBinding
  attr_accessor :condition
  attr_accessor :hidden

  def initialize(name, opts)
    @name = name
    @type = opts["type"]
    @defaultValue = opts["defaultValue"]
    @humanReadableName = '"' + (opts["humanReadableName"] || "") + '"'
    @humanReadableDescription = '"' + (opts["humanReadableDescription"] || "") + '"'
    @getter = opts["getter"]
    @webcoreBinding = opts["webcoreBinding"]
    @webcoreName = opts["webcoreName"]
    @condition = opts["condition"]
    @hidden = opts["hidden"] || false
  end

  def nameLower
    if @getter
      @getter
    elsif @name.start_with?("VP")
      @name[0..1].downcase + @name[2..@name.length]
    elsif @name.start_with?("CSS", "XSS", "FTP", "DOM", "DNS", "PDF", "ICE")
      @name[0..2].downcase + @name[3..@name.length]
    elsif @name.start_with?("HTTP")
      @name[0..3].downcase + @name[4..@name.length]
    else
      @name[0].downcase + @name[1..@name.length]
    end
  end

  def webcoreNameUpper
    if @webcoreName
      @webcoreName[0].upcase + @webcoreName[1..@webcoreName.length]
    else
      @name
    end
  end

  def typeUpper
    if @type == "uint32_t"
      "UInt32"
    else
      @type.capitalize
    end
  end
end

class Conditional
  attr_accessor :condition
  attr_accessor :preferences

  def initialize(condition, settings)
    @condition = condition
    @preferences = preferences
  end
end

class Preferences
  attr_accessor :preferences

  def initialize(parsedBasePreferences, parsedDebugPreferences, parsedExperimentalPreferences, parsedInternalPreferences, outputDirectory)
    @outputDirectory = outputDirectory

    @preferences = []
    @preferencesNotDebug = []
    @preferencesDebug = []
    @experimentalFeatures = []
    @internalDebugFeatures = []

    if parsedBasePreferences
      parsedBasePreferences.each do |name, options|
        preference = Preference.new(name, options)
        @preferences << preference
        @preferencesNotDebug << preference
      end
    end
    if parsedDebugPreferences
      parsedDebugPreferences.each do |name, options|
        preference = Preference.new(name, options)
        @preferences << preference
        @preferencesDebug << preference
      end
    end
    if parsedExperimentalPreferences
      parsedExperimentalPreferences.each do |name, options|
        preference = Preference.new(name, options)
        @preferences << preference
        @experimentalFeatures << preference
      end
    end
    if parsedInternalPreferences
      parsedInternalPreferences.each do |name, options|
        preference = Preference.new(name, options)
        @preferences << preference
        @internalDebugFeatures << preference
      end
    end

    @preferences.sort! { |x, y| x.name <=> y.name }
    @preferencesNotDebug.sort! { |x, y| x.name <=> y.name }
    @preferencesDebug.sort! { |x, y| x.name <=> y.name }
    @experimentalFeatures.sort! { |x, y| x.name <=> y.name }.sort! { |x, y| x.humanReadableName <=> y.humanReadableName }
    @internalDebugFeatures.sort! { |x, y| x.name <=> y.name }.sort! { |x, y| x.humanReadableName <=> y.humanReadableName }

    @preferencesBoundToSetting = @preferences.select { |p| !p.webcoreBinding }
    @preferencesBoundToDeprecatedGlobalSettings = @preferences.select { |p| p.webcoreBinding == "DeprecatedGlobalSettings" }
    @preferencesBoundToRuntimeEnabledFeatures = @preferences.select { |p| p.webcoreBinding == "RuntimeEnabledFeatures" }

    @warning = "THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT."
  end

  def renderTemplate(template)
    templateFile = File.join(File.dirname(__FILE__), "PreferencesTemplates", template + ".erb")

    output = ERB.new(File.read(templateFile), 0, "-").result(binding)
    File.open(File.join(@outputDirectory, template), "w+") do |f|
      f.write(output)
    end
  end
end

preferences = Preferences.new(parsedBasePreferences, parsedDebugPreferences, parsedExperimentalPreferences, parsedInternalPreferences, options[:outputDirectory])
preferences.renderTemplate("WebPreferencesDefinitions.h")
preferences.renderTemplate("WebPreferencesExperimentalFeatures.mm")
preferences.renderTemplate("WebViewPreferencesChangedGenerated.mm")
