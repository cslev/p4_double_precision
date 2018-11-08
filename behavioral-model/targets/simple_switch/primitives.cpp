/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/calculations.h>
#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_sim/counters.h>
#include <bm/bm_sim/meters.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/logger.h>

#include <random>
#include <thread>

// -- LEVI for double
#include <stdlib.h>     /* atof */
#include <iostream>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <cstdio>
// -- END LEVI

template <typename... Args>
using ActionPrimitive = bm::ActionPrimitive<Args...>;

using bm::Data;
using bm::Field;
using bm::Header;
using bm::MeterArray;
using bm::CounterArray;
using bm::RegisterArray;
using bm::NamedCalculation;
using bm::HeaderStack;
using bm::PHV;

class modify_field : public ActionPrimitive<Data &, const Data &> {
  void operator ()(Data &dst, const Data &src) {
    bm::core::assign()(dst, src);
  }
};

REGISTER_PRIMITIVE(modify_field);

class modify_field_rng_uniform
  : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &b, const Data &e) {
    // TODO(antonin): a little hacky, fix later if there is a need using GMP
    // random fns
    using engine = std::default_random_engine;
    using hash = std::hash<std::thread::id>;
    static thread_local engine generator(hash()(std::this_thread::get_id()));
    using distrib64 = std::uniform_int_distribution<uint64_t>;
    distrib64 distribution(b.get_uint64(), e.get_uint64());
    f.set(distribution(generator));
  }
};

REGISTER_PRIMITIVE(modify_field_rng_uniform);

class add_to_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.add(f, d);
  }
};

REGISTER_PRIMITIVE(add_to_field);

class subtract_from_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.sub(f, d);
  }
};

REGISTER_PRIMITIVE(subtract_from_field);

class add : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.add(d1, d2);
  }
};

REGISTER_PRIMITIVE(add);

class subtract : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.sub(d1, d2);
  }
};

REGISTER_PRIMITIVE(subtract);

class bit_xor : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.bit_xor(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_xor);

class bit_or : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.bit_or(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_or);

class bit_and : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.bit_and(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_and);

class shift_left :
  public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.shift_left(d1, d2);
  }
};

REGISTER_PRIMITIVE(shift_left);

class shift_right :
  public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &f, const Data &d1, const Data &d2) {
    f.shift_right(d1, d2);
  }
};

REGISTER_PRIMITIVE(shift_right);

class drop : public ActionPrimitive<> {
  void operator ()() {
    get_field("standard_metadata.egress_spec").set(511);
    if (get_phv().has_field("intrinsic_metadata.mcast_grp")) {
      get_field("intrinsic_metadata.mcast_grp").set(0);
    }
  }
};
REGISTER_PRIMITIVE(drop);



// -- LEVI
class p4_logger :
  public ActionPrimitive<const Data &> {
    void operator()(const Data &operand) {
      std::string l  = "\033[1;34m[P4 logger]\t";
      std::string ll = "\033[0m\n";
      std::stringstream stream;
      stream << std::hex << operand.get_uint64();
      std::string result(stream.str());

      std::cout << l << result << ll;
    }
  };
REGISTER_PRIMITIVE(p4_logger);

class double_to_int64 :
  public ActionPrimitive<Data &, const Data &, const Data &> {
    void operator()(Data &int64_num, const Data &double_number_as_hex, const Data &precision) {
      std::string l  = "\033[1;35m[DOUBLE_TO_INT64]\t";
      std::string ll = "\033[0m\n";
      std::stringstream stream;
      stream << std::hex << double_number_as_hex.get_uint64();
      std::string result(stream.str());

      double double_as_double = rawStringToDouble(result.c_str());
      int prec = precision.get_int();
      int64_t tmp_int64_num = (int64_t)(double_as_double*prec);

      std::cout << l << "received double in hex: " << result << ll;
      std::cout << l << "received double as double: " << double_as_double << ll;
      std::cout << l << "precision:" << prec << ll;
      std::cout << l << "Big int (double*precision): " << tmp_int64_num << ll;

      std::stringstream stream2;
      stream2 << std::setfill ('0') << std::setw(16) << std::hex << tmp_int64_num;
      std::string result_int(stream2.str());
      std::cout << l << "Big int (double*precision) as hex: " << result_int << ll;
      int64_num.set(result_int);
    }

  private:
    double rawStringToDouble(const char *input)
    {
        const size_t bytesInDouble = 8;
        union {
            double value;
            unsigned char bytes[bytesInDouble];
        } u;
        unsigned char *output = u.bytes;
        for(uint i = 0; i < bytesInDouble; ++i) {
            sscanf(input, "%02hhX", output);
            input += 2;
            ++output;
        }
        return u.value;
    }

  };
REGISTER_PRIMITIVE(double_to_int64);

class int64_to_double :
  public ActionPrimitive<Data &, const Data &, const Data &> {
    void operator()(Data &double_num, const Data &int64_as_hex, const Data &precision) {
      std::string l  = "\033[1;33m[INT64_TO_DOUBLE]\t";
      std::string ll = "\033[0m\n";
      std::stringstream stream;
      stream << std::setfill ('0') << std::setw(16) << std::hex << int64_as_hex.get_uint64();
      std::string result(stream.str());

      int64_t int64_as_int = int64_as_hex.get_uint64();
      int prec = precision.get_int();
      double tmp_double_num = (double)((double)int64_as_int/prec);

      std::cout << l << "received int in hex: " << result << ll;
      std::cout << l << "received int as int: " << int64_as_int << ll;
      std::cout << l << "precision:" << prec << ll;
      std::cout << l << "Double (int/precision): " << tmp_double_num << ll;

      std::string double_as_hex = double2hexstr(tmp_double_num);
      std::cout << l << "Double (int/precision) as hex: " << double_as_hex << ll;
      double_num.set(double_as_hex);
    }

  private:
    std::string double2hexstr(double x)
    {
      union
        {
            long long i;
            double    d;
        } value;
       value.d = change_endian(x);
       std::ostringstream buf;
       buf << std::hex << std::setfill('0') << std::setw(16) << value.i;
       return buf.str();
    }

    template <class T>
    T change_endian(T in)
    {
        char* const p = reinterpret_cast<char*>(&in);
        for (size_t i = 0; i < sizeof(T) / 2; ++i)
            std::swap(p[i], p[sizeof(T) - i - 1]);
        return in;
    }
  };
REGISTER_PRIMITIVE(int64_to_double);

// -- END LEVI


class exit_ : public ActionPrimitive<> {
  void operator ()() {
    get_packet().mark_for_exit();
  }
};

REGISTER_PRIMITIVE_W_NAME("exit", exit_);

class generate_digest : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &receiver, const Data &learn_id) {
    // discared receiver for now
    (void) receiver;
    get_field("intrinsic_metadata.lf_field_list").set(learn_id);
  }
};

REGISTER_PRIMITIVE(generate_digest);

class add_header : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    // TODO(antonin): reset header to 0?
    if (!hdr.is_valid()) {
      hdr.reset();
      hdr.mark_valid();
      // updated the length packet register (register 0)
      auto &packet = get_packet();
      packet.set_register(0, packet.get_register(0) + hdr.get_nbytes_packet());
    }
  }
};

REGISTER_PRIMITIVE(add_header);

class add_header_fast : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    hdr.mark_valid();
  }
};

REGISTER_PRIMITIVE(add_header_fast);

class remove_header : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    if (hdr.is_valid()) {
      // updated the length packet register (register 0)
      auto &packet = get_packet();
      packet.set_register(0, packet.get_register(0) - hdr.get_nbytes_packet());
      hdr.mark_invalid();
    }
  }
};

REGISTER_PRIMITIVE(remove_header);

class copy_header : public ActionPrimitive<Header &, const Header &> {
  void operator ()(Header &dst, const Header &src) {
    bm::core::assign_header()(dst, src);
  }
};

REGISTER_PRIMITIVE(copy_header);

/* standard_metadata.clone_spec will contain the mirror id (16 LSB) and the
   field list id to copy (16 MSB) */
class clone_ingress_pkt_to_egress
  : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &clone_spec, const Data &field_list_id) {
    Field &f_clone_spec = get_field("standard_metadata.clone_spec");
    f_clone_spec.shift_left(field_list_id, 16);
    f_clone_spec.add(f_clone_spec, clone_spec);
  }
};

REGISTER_PRIMITIVE(clone_ingress_pkt_to_egress);

class clone_egress_pkt_to_egress
  : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &clone_spec, const Data &field_list_id) {
    Field &f_clone_spec = get_field("standard_metadata.clone_spec");
    f_clone_spec.shift_left(field_list_id, 16);
    f_clone_spec.add(f_clone_spec, clone_spec);
  }
};

REGISTER_PRIMITIVE(clone_egress_pkt_to_egress);

class resubmit : public ActionPrimitive<const Data &> {
  void operator ()(const Data &field_list_id) {
    if (get_phv().has_field("intrinsic_metadata.resubmit_flag")) {
      get_phv().get_field("intrinsic_metadata.resubmit_flag")
          .set(field_list_id);
    }
  }
};

REGISTER_PRIMITIVE(resubmit);

class recirculate : public ActionPrimitive<const Data &> {
  void operator ()(const Data &field_list_id) {
    if (get_phv().has_field("intrinsic_metadata.recirculate_flag")) {
      get_phv().get_field("intrinsic_metadata.recirculate_flag")
          .set(field_list_id);
    }
  }
};

REGISTER_PRIMITIVE(recirculate);

class modify_field_with_hash_based_offset
  : public ActionPrimitive<Data &, const Data &,
                           const NamedCalculation &, const Data &> {
  void operator ()(Data &dst, const Data &base,
                   const NamedCalculation &hash, const Data &size) {
    uint64_t v =
      (hash.output(get_packet()) % size.get<uint64_t>()) + base.get<uint64_t>();
    dst.set(v);
  }
};

REGISTER_PRIMITIVE(modify_field_with_hash_based_offset);

class no_op : public ActionPrimitive<> {
  void operator ()() {
    // nothing
  }
};

REGISTER_PRIMITIVE(no_op);

class execute_meter
  : public ActionPrimitive<MeterArray &, const Data &, Field &> {
  void operator ()(MeterArray &meter_array, const Data &idx, Field &dst) {
    auto i = idx.get_uint();
#ifndef NDEBUG
    if (i >= meter_array.size()) {
        BMLOG_ERROR_PKT(get_packet(),
                        "Attempted to update meter '{}' with size {}"
                        " at out-of-bounds index {}."
                        "  No meters were updated, and neither was"
                        " dest field.",
                        meter_array.get_name(), meter_array.size(), i);
        return;
    }
#endif  // NDEBUG
    auto color = meter_array.execute_meter(get_packet(), i);
    dst.set(color);
    BMLOG_TRACE_PKT(get_packet(),
                    "Updated meter '{}' at index {},"
                    " assigning dest field the color result {}",
                    meter_array.get_name(), i, color);
  }
};

REGISTER_PRIMITIVE(execute_meter);

class count : public ActionPrimitive<CounterArray &, const Data &> {
  void operator ()(CounterArray &counter_array, const Data &idx) {
    auto i = idx.get_uint();
#ifndef NDEBUG
    if (i >= counter_array.size()) {
        BMLOG_ERROR_PKT(get_packet(),
                        "Attempted to update counter '{}' with size {}"
                        " at out-of-bounds index {}."
                        "  No counters were updated.",
                        counter_array.get_name(), counter_array.size(), i);
        return;
    }
#endif  // NDEBUG
    counter_array.get_counter(i).increment_counter(get_packet());
    BMLOG_TRACE_PKT(get_packet(),
                    "Updated counter '{}' at index {}",
                    counter_array.get_name(), i);
  }
};

REGISTER_PRIMITIVE(count);

class register_read
  : public ActionPrimitive<Field &, const RegisterArray &, const Data &> {
  void operator ()(Field &dst, const RegisterArray &src, const Data &idx) {
    auto i = idx.get_uint();
#ifndef NDEBUG
    if (i >= src.size()) {
        BMLOG_ERROR_PKT(get_packet(),
                        "Attempted to read register '{}' with size {}"
                        " at out-of-bounds index {}."
                        "  Dest field was not updated.",
                        src.get_name(), src.size(), i);
        return;
    }
#endif  // NDEBUG
    dst.set(src[i]);
    BMLOG_TRACE_PKT(get_packet(),
                    "Read register '{}' at index {} read value {}",
                    src.get_name(), i, src[i]);
  }
};

REGISTER_PRIMITIVE(register_read);

class register_write
  : public ActionPrimitive<RegisterArray &, const Data &, const Data &> {
  void operator ()(RegisterArray &dst, const Data &idx, const Data &src) {
    auto i = idx.get_uint();
#ifndef NDEBUG
    if (i >= dst.size()) {
        BMLOG_ERROR_PKT(get_packet(),
                        "Attempted to write register '{}' with size {}"
                        " at out-of-bounds index {}."
                        "  No register array elements were updated.",
                        dst.get_name(), dst.size(), i);
        return;
    }
#endif  // NDEBUG
    dst[i].set(src);
    BMLOG_TRACE_PKT(get_packet(),
                    "Wrote register '{}' at index {} with value {}",
                    dst.get_name(), i, dst[i]);
  }
};

REGISTER_PRIMITIVE(register_write);

// I cannot name this "truncate" and register it with the usual
// REGISTER_PRIMITIVE macro, because of a name conflict:
//
// In file included from /usr/include/boost/config/stdlib/libstdcpp3.hpp:77:0,
//   from /usr/include/boost/config.hpp:44,
//   from /usr/include/boost/cstdint.hpp:36,
//   from /usr/include/boost/multiprecision/number.hpp:9,
//   from /usr/include/boost/multiprecision/gmp.hpp:9,
//   from ../../src/bm_sim/include/bm_sim/bignum.h:25,
//   from ../../src/bm_sim/include/bm_sim/data.h:32,
//   from ../../src/bm_sim/include/bm_sim/fields.h:28,
//   from ../../src/bm_sim/include/bm_sim/phv.h:34,
//   from ../../src/bm_sim/include/bm_sim/actions.h:34,
//   from primitives.cpp:21:
//     /usr/include/unistd.h:993:12: note: declared here
//     extern int truncate (const char *__file, __off_t __length)
class truncate_ : public ActionPrimitive<const Data &> {
  void operator ()(const Data &truncated_length) {
    get_packet().truncate(truncated_length.get<size_t>());
  }
};

REGISTER_PRIMITIVE_W_NAME("truncate", truncate_);

// dummy function, which ensures that this unit is not discarded by the linker
// it is being called by the constructor of SimpleSwitch
// the previous alternative was to have all the primitives in a header file (the
// primitives could also be placed in simple_switch.cpp directly), but I need
// this dummy function if I want to keep the primitives in their own file
int import_primitives() {
  return 0;
}
