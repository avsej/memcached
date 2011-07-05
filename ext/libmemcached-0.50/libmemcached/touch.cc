/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libmemcached/common.h>
#include <libmemcached/memcached/protocol_binary.h>

memcached_return_t memcached_touch(memcached_st *ptr,
                                   const char *key, size_t key_length,
                                   time_t expiration)
{
  return memcached_touch_by_key(ptr, key, key_length, key, key_length, expiration);
}

memcached_return_t memcached_touch_by_key(memcached_st *ptr,
                                          const char *group_key, size_t group_key_length,
                                          const char *key, size_t key_length,
                                          time_t expiration)
{
  uint32_t server_key;
  memcached_return_t rc;
  memcached_server_write_instance_st instance;

  LIBMEMCACHED_MEMCACHED_TOUCH_START();

  unlikely ((ptr->flags.binary_protocol) == false)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);
  }

  unlikely (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  unlikely (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  unlikely (memcached_failed(rc= memcached_validate_key_length(group_key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  protocol_binary_request_touch request= {}; //{.bytes= {0}};
  size_t prefix_key_length = memcached_array_size(ptr->prefix_key);
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_TOUCH;
  request.message.header.request.extlen= 4;
  request.message.header.request.keylen= htons((uint16_t)(key_length + prefix_key_length));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + prefix_key_length + request.message.header.request.extlen));
  request.message.body.expiration= htonl((uint32_t) expiration);

  struct libmemcached_io_vector_st vector[]=
  {
    { sizeof(request.bytes), request.bytes },
    { prefix_key_length, memcached_array_string(ptr->prefix_key) },
    { key_length, key }
  };

  if (memcached_failed(rc= memcached_vdo(instance, vector, 3, true)))
  {
    memcached_io_reset(instance);
    return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
  }

  return memcached_read_one_response(instance, NULL, 0, &ptr->result);
}
