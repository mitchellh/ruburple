
home = File.expand_path(File.dirname(__FILE__))
$: << File.join(home, "..", "lib")

require 'pp'
require 'test/unit'
require 'benchmark'
require 'ruburple'
require 'pathname'

def bm(label = "", options = {})
  n = options[:n] || 1000
  width = options[:width] || 50
  Benchmark.benchmark(" " * width + Benchmark::Tms::CAPTION, width, Benchmark::Tms::FMTSTR, "ms/call") do |b|
    times = b.report("#{n}x#{label}") do
      n.times do
        yield
      end
    end
    [times * 1000 / n.to_f]
  end
end

class TestAccount
  attr_accessor :protocol, :username, :password
  def initialize(protocol, username, password)
    @protocol = protocol
    @username = username
    @password = password
  end
end

def die(s)
  puts "*** #{s} ***"
  exit 1
end

class Test::Unit::TestCase

  def assert_within(timeout, &block)
    t = Time.new
    rval = yield
    while !rval && t > Time.new - timeout
      rval = yield
      sleep(0.05)
    end
    assert(rval)
  end

end
