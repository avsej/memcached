class Memcached
  module Experimental

    # TOUCH is used to set a new expiration time for an existing item
    def touch(key, ttl=@default_ttl)
      check_return_code(
        Lib.memcached_touch(@struct, key, ttl),
        key
      )
    rescue => e
      tries ||= 0
      raise unless should_retry(tries, e)
      tries += 1
      retry
    end

    def get_len(bytes, keys)
      if keys.is_a? ::Array
        # Multi get
        ret = Lib.memcached_mget_len(@struct, keys, bytes);
        check_return_code(ret, keys)

        hash = {}
        loop do
          value, key, flags, ret = Lib.memcached_fetch_rvalue(@struct)
          break if value.nil? || ret == Lib::MEMCACHED_END
          if ret == Lib::MEMCACHED_SUCCESS
            hash[key] = value
          elsif ret != Lib::MEMCACHED_NOTFOUND
            check_return_code(ret, key)
          end
        end
        hash
      else
        # Single get_len
        value, flags, ret = Lib.memcached_get_len_rvalue(@struct, keys, bytes)
        check_return_code(ret, keys)
        value
      end
    rescue => e
      tries ||= 0
      raise unless should_retry(tries, e)
      tries += 1
      retry
    end

  end
end
