Gem::Specification.new do |s|
    s.platform  =   Gem::Platform::RUBY
    s.name      =   "ruburple"
    s.version   =   "0.0.8"
    s.author    =   "Martin Kihlgren"
    s.email     =   "zond at troja dot ath dot cx"
    s.summary   =   "An ruby extension to libpurple."
    s.files     =   Dir['lib/**/*.rb'] + Dir['tests/*'] + Dir['ext/*'] + ['LICENSE']
    s.require_path  =   "lib"
    s.extensions << 'ext/extconf.rb'
    s.autorequire   =   "ruburple"
    s.test_files = Dir.glob('tests/*_test.rb') + Dir.glob('tests/test_helper.rb')
    s.has_rdoc  =   true
    s.rdoc_options << '--line-numbers'
    s.rdoc_options << '--inline-source'
    s.extra_rdoc_files  =   ["README"]
end