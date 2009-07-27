Gem::Specification.new do |s|
    s.platform  =   Gem::Platform::RUBY
    s.name      =   "ruburple"
    s.version   =   "0.0.8.1"
    s.author    =   "Martin Kihlgren"
    s.email     =   "zond at troja dot ath dot cx"
    s.summary   =   "An ruby extension to libpurple."
    s.files     =   Dir.glob("{ext,lib,tests}/**/*")
    s.files.concat ["LICENSE", "README"]
    s.require_path  =   "lib"
    s.extensions << 'ext/extconf.rb'

    s.test_files = Dir.glob('tests/*_test.rb') + Dir.glob('tests/test_helper.rb')
    s.has_rdoc = false
end