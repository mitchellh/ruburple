
require File.join(File.dirname(__FILE__), 'test_helper')
require 'ruburple'

class RuburpleTest < Test::Unit::TestCase

  accounts_file = Pathname.new(__FILE__).parent.join("test_accounts.rb")
  die("You have to create a #{accounts_file} looking like #{accounts_file}.template to run #{__FILE__}") unless accounts_file.exist?
  require accounts_file

  @@events = {}

  puts "testing init..."
  Ruburple::init
  puts "testing subscribe..."
  Ruburple::subscribe(:signed_on) do |c| 
    puts "got signed_on from #{c.account.username}"
    @@events[:signed_on] << c
  end
  Ruburple::subscribe(:received_im_msg) do |account, sender, message, conversation, flags| 
    puts "got received_im_msg from #{sender} to #{account.username}"
    @@events[:received_im_msg] << [account.username, sender, message]
  end

  def setup
    @@events = {
      :signed_on => [],
      :received_im_msg => []
    }
  end

  def test_accounts
    $TEST_ACCOUNTS.each do |ac1, ac2|
      test_operations(ac1, ac2)
    end
  end

  private

  def test_operations(ac1, ac2)
    puts "testing #{ac1.protocol} + #{ac2.protocol}..."
    assert(Ruburple::has_protocol?(ac1.protocol))
    assert(Ruburple::has_protocol?(ac2.protocol))
    p1 = Ruburple::get_protocol(ac1.protocol)
    p2 = Ruburple::get_protocol(ac2.protocol)
    puts "testing get account..."
    a1 = p1.get_account(ac1.username, ac1.password)
    a2 = p2.get_account(ac2.username, ac2.password)
    puts "testing connecting..."
    assert(!a1.connecting?)
    a1.connect
    assert(a1.connecting?)
    puts "testing sign on..."
    assert_within(30) do 
      @@events[:signed_on].find do |e|
        e.account.username == a1.username
      end
    end
    assert(!a1.connecting?)
    assert(a1.connected?)
    puts "testing uid..."
    assert(a1.uid != a2.uid)
    assert_equal(a1.uid, a1.connection.account.uid)
    sleep 2
    puts "testing buddies..."
    assert(a1.has_buddy?(a2.username))
    assert(buddy2 = a1.buddies.find do |b|
             a2.username =~ /#{Regexp.escape(b.name)}/
           end)
    puts "testing set enabled..."
    a2.enabled = true
    assert_within(30) do 
      @@events[:signed_on].find do |e|
        e.account.username == a2.username
      end
    end
    a1.savedstatus = Ruburple::Protocol::Account::SavedStatus.new(Ruburple::Protocol::Account::Status::STATUS_AVAILABLE)
    a2.savedstatus = Ruburple::Protocol::Account::SavedStatus.new(Ruburple::Protocol::Account::Status::STATUS_AVAILABLE)
    puts "testing status..."
    assert_equal("Available", a1.status.name)
    assert_equal("Available", a2.status.name)
    assert_equal(Ruburple::Protocol::Account::Status::STATUS_AVAILABLE, a1.status.primitive)
    assert_equal(Ruburple::Protocol::Account::Status::STATUS_AVAILABLE, a2.status.primitive)
    puts "testing buddies..."
    sleep 2
    assert(a2.has_buddy?(a1.username))
    assert(buddy1 = a2.buddies.find do |b|
             a1.username =~ /#{Regexp.escape(b.name)}/
           end)
    puts "testing send im..."
    a1.connection.send_im(a2.username, "hello a2")
    a2.connection.send_im(a1.username, "hello a1")
    assert_within(30) do 
      a1_got_message = @@events[:received_im_msg].find do |recipient, sender, message|
        recipient == a1.username && sender = a2.username && message =~ /hello a1/
      end
      a2_got_message = @@events[:received_im_msg].find do |recipient, sender, message|
        recipient == a2.username && sender = a1.username && message =~ /hello a2/
      end
      a1_got_message && a2_got_message
    end
    puts "testing set enabled..."
    a1.enabled = false
    puts "testing close..."
    a2.connection.close
  end
end
