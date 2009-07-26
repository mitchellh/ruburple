
require 'rake'
require 'rake/testtask'
require 'rubygems'
Gem::manage_gems
require 'rake/gempackagetask'


spec = Gem::Specification.new do |s|
    s.platform  =   Gem::Platform::RUBY
    s.name      =   "ruburple"
    s.version   =   "0.0.8"
    s.author    =   "Martin Kihlgren"
    s.email     =   "zond at troja dot ath dot cx"
    s.summary   =   "An ruby extension to libpurple."
    s.files     =   FileList['lib/**/*.rb', 'tests/*', 'ext/*', 'LICENSE'].to_a
    s.require_path  =   "lib"
    s.extensions << 'ext/extconf.rb'
    s.autorequire   =   "ruburple"
    s.test_files = Dir.glob('tests/*_test.rb') + Dir.glob('tests/test_helper.rb')
    s.has_rdoc  =   true
    s.rdoc_options << '--line-numbers'
    s.rdoc_options << '--inline-source'
    s.extra_rdoc_files  =   ["README"]
end


SOURCE_FILES = FileList.new do |fl|
  [ "lib", "tests" ].each do |dir|
      fl.include "#{dir}/**/*"
  end
  fl.include "Rakefile"
end

Rake::GemPackageTask.new(spec) do |pkg|
    pkg.need_tar = true
end

task :default => [:units] do
end

desc "Run all tests"
Rake::TestTask.new(:units) do |t|
  	t.pattern = 'tests/*_test.rb'
  	t.verbose = true
  	t.warning = true
end

desc "Run all benchmarks"
Rake::TestTask.new(:bench) do |t|
  	t.pattern = 'tests/*_benchmark.rb'
  	t.verbose = true
  	t.warning = true
end

desc "Package a gem from the source"
task :gem => "pkg/#{spec.name}-#{spec.version}.gem" do
    puts "generated latest version"
end

