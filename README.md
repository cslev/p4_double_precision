# p4_double_precision
This repository contains an updated p4c compiler and a corresponding BMv2 software switch, which can handle Double precision numbers.

To avoid forking back-and-forth github projects from `p4lang` repositories, I have manually added `p4c` and `behavioral-model` sources to this repository and made the required changes to them. Therefore, the credits for the original repositories go for the `p4lang` team.

# What this repository can give you
 - A simple logging function that can print out variables to the standard out (e.g., when `--log-console` is set for `simple_switch`)
 - a simple functions (`double_to_int64()`, `int64_to_double()`) for handling double numbers in a **slightly interesting way**:
 
The basic idea behind handling and making operations with double numbers is to avoid adding complete set of double precision functions as an extern. Instead, I thought that an extern function that can convert a double to an integer number (uint64) via multiplying it with a *precision* parameter as a power of 10 (e.g., number of digits after the comma), the switch would be able to make the basic operations as it would do in case of the well-known integer numbers. Once calculations are done (e.g., subtracting two numbers), another extern function will convert the result back to a double number by again setting a *precision* parameter. Finally, the function pass back the hexadecimal/binary representation of the double number (not the double-typed variable of course) so the P4 application can easily deal with it later on (e.g., setting it as a header value). 

It sounds like the switch becomes a little bit dumb as it only knows the operator but not the **real** operands and just do the calculations without knowing what “exactly is being done”. For many of the double precision number related problems it is more than sufficient (at least it was enough for me at the moment:)). On the other hand, since double operators are not needed to be implemented, it might be easier to adapt such an approach in future P4 switch releases.
The only drawback (at the moment) is that we cannot define a double number without knowing its hexadecimal/binary representation (not to mention the endianness :(). But so far, it worked as it should even for negative numbers.


 
## Usage example
```
bit<64> my_double_1_1 = 0x9a9999999999f13f; //1.1
bit<64> my_double_m1_1 = 0x9a9999999999f1bf; //-1.1
bit<64> my_double_0_2 = 0x9a9999999999c93f; //0.2
p4_logger(my_double_1_1);
p4_logger(my_double_m1_1);
p4_logger(my_double_0_2);
//convert them to int64 with precision = 100 (2)
bit<64> my_converted_double_1_1_int;
bit<64> my_converted_double_m1_1_int;
bit<64> my_converted_double_0_2_int;

//to avoid power functions in the extern, precision should be defined via already having the power function calculated
double_to_int64(my_converted_double_1_1_int, my_double_1_1, (bit<64>)100);
double_to_int64(my_converted_double_m1_1_int, my_double_m1_1, (bit<64>)100);
double_to_int64(my_converted_double_0_2_int, my_double_0_2, (bit<64>)100);

//values are in HEX!
//Do the operations on the int64 numbers
//Subtracting
bit<64> res = my_converted_double_1_1_int - my_converted_double_0_2_int;
p4_logger(res);
//Subtract from negative number
bit<64> mres = my_converted_double_m1_1_int - my_converted_double_0_2_int;    
p4_logger(mres);

//Multiplication 
bit<64> multiply_res = my_converted_double_1_1_int * my_converted_double_0_2_int;
bit<64> mmultiply_res = my_converted_double_m1_1_int * my_converted_double_0_2_int;

//converting back the results to double
bit<64> res_double;
int64_to_double(res_double, res, (bit<64>)100);
p4_logger(res_double);
bit<64> mres_double;
int64_to_double(mres_double, mres, (bit<64>)100);
p4_logger(mres_double);

//for multiplication we need extra care as precision values need to be multiplied as well
bit<64> multiply_res_double;
int64_to_double(multiply_res_double, multiply_res, (bit<64>)(10000));
p4_logger(multiply_res_double);

bit<64> mmultiply_res_double;
int64_to_double(mmultiply_res_double, mmultiply_res, (bit<64>)(10000));
p4_logger(mmultiply_res_double);
```

In the above example, we define 3 double precision number via their 8-byte hexadecimal representation<sup>[1](#myfootnote1)</sup>, in particular 1.1, -1.1, 0.2. Then, we convert each of them to an int64 representation with the precision set to 100 (i.e., to 2). As one can observe, in order to avoid costly power function, precision should be set by already doing the powering function in your head (this could be another approach as well). Next, we subtract 0.2 from 1.1, in particular, we subtract the int64 representation of 0.2 (i.e., 20) from the int64 representation of 1.1 (i.e., 110). Similarly, we also subtract 0.2 from -1.1 to see whether negative numbers are also working properly. Once, the subtractions are made, we convert the int64 values back to double precision numbers and stores their hexadecimal/binary representation. To check other operator, we also multiply 1.1 with 0.2 and 1.1 with -0.2. Note that in case of multiplication the precision values are also need to be multiplied to get the correct value, i.e., if for 1.1 and 0.2 we use a precision value of 100, then for the conversion we need to use a precision value of 10000. The corresponding output can be seen below (outputs are printed via std::cout calls in the extern functions):
```
[DOUBLE_TO_INT64]    received double in hex: 9a9999999999f13f
[DOUBLE_TO_INT64]    received double as double: 1.1
[DOUBLE_TO_INT64]    precision:100
[DOUBLE_TO_INT64]    Big int (double*precision): 110
[DOUBLE_TO_INT64]    Big int (double*precision) as hex: 000000000000006e
[17:52:35.916] [bmv2] [T] [thread 417] [0.0] [cxt 0] ../../p4debug/monitoring.p4(544) Primitive double_to_int64(my_converted_double_m1_1_int, my_double_m1_1, (bit<64>)100)
[DOUBLE_TO_INT64]    received double in hex: 9a9999999999f1bf
[DOUBLE_TO_INT64]    received double as double: -1.1
[DOUBLE_TO_INT64]    precision:100
[DOUBLE_TO_INT64]    Big int (double*precision): -110
[DOUBLE_TO_INT64]    Big int (double*precision) as hex: ffffffffffffff92
[17:52:35.916] [bmv2] [T] [thread 417] [0.0] [cxt 0] ../../p4debug/monitoring.p4(545) Primitive double_to_int64(my_converted_double_0_2_int, my_double_0_2, (bit<64>)100)
[DOUBLE_TO_INT64]    received double in hex: 9a9999999999c93f
[DOUBLE_TO_INT64]    received double as double: 0.2
[DOUBLE_TO_INT64]    precision:100
[DOUBLE_TO_INT64]    Big int (double*precision): 20
[DOUBLE_TO_INT64]    Big int (double*precision) as hex: 0000000000000014
[INT64_TO_DOUBLE]    received int in hex: 000000000000005a
[INT64_TO_DOUBLE]    received int as int: 90
[INT64_TO_DOUBLE]    precision:100
[INT64_TO_DOUBLE]    Double (int/precision): 0.9
[INT64_TO_DOUBLE]    Double (int/precision) as hex: cdccccccccccec3f
[17:52:35.916] [bmv2] [T] [thread 417] [0.0] [cxt 0] ../../p4debug/monitoring.p4(556) Primitive p4_logger(res_double)
[P4 logger]    cdccccccccccec3f
[17:52:35.916] [bmv2] [T] [thread 417] [0.0] [cxt 0] ../../p4debug/monitoring.p4(558) Primitive int64_to_double(mres_double, mres, (bit<64>)100)
[INT64_TO_DOUBLE]    received int in hex: ffffffffffffff7e
[INT64_TO_DOUBLE]    received int as int: -130
[INT64_TO_DOUBLE]    precision:100
[INT64_TO_DOUBLE]    Double (int/precision): -1.3
[INT64_TO_DOUBLE]    Double (int/precision) as hex: cdccccccccccf4bf
[17:52:35.916] [bmv2] [T] [thread 417] [0.0] [cxt 0] ../../p4debug/monitoring.p4(559) Primitive p4_logger(mres_double)
[P4 logger]    cdccccccccccf4bf
[INT64_TO_DOUBLE]    received int in hex: 0000000000000898
[INT64_TO_DOUBLE]    received int as int: 2200
[INT64_TO_DOUBLE]    precision:10000
[INT64_TO_DOUBLE]    Double (int/precision): 0.22
[INT64_TO_DOUBLE]    Double (int/precision) as hex: 295c8fc2f528cc3f
[18:12:57.908] [bmv2] [T] [thread 1229] [33.0] [cxt 0] ../../p4debug/monitoring.p4(564) Primitive p4_logger(multiply_res_double)
[P4 logger]    295c8fc2f528cc3f
[18:12:57.908] [bmv2] [T] [thread 1229] [33.0] [cxt 0] ../../p4debug/monitoring.p4(567) Primitive int64_to_double(mmultiply_res_double, mmultiply_res, (bit<64>)(10000))
[INT64_TO_DOUBLE]    received int in hex: fffffffffffff768
[INT64_TO_DOUBLE]    received int as int: -2200
[INT64_TO_DOUBLE]    precision:10000
[INT64_TO_DOUBLE]    Double (int/precision): -0.22
[INT64_TO_DOUBLE]    Double (int/precision) as hex: 295c8fc2f528ccbf
[18:12:57.908] [bmv2] [T] [thread 1229] [33.0] [cxt 0] ../../p4debug/monitoring.p4(568) Primitive p4_logger(mmultiply_res_double)
[P4 logger]    295c8fc2f528ccbf
```
<a name="myfootnote1"><sup>1</sup></a> Use this online tool to get your number for testing: [https://gregstoll.dyndns.org/~gregstoll/floattohex/](https://gregstoll.dyndns.org/~gregstoll/floattohex/) (take care of the endianness -> networking is big-endian, but x86 architecture is little-endian)

# Compiling
For compiling the sources you don't need anything special, just compile the compiler ([p4c](https://github.com/p4lang/p4c)) and the software switch ([behavioral-model](https://github.com/p4lang/behavioral-model)) according to their basic instructions.

# You want to add your own extern?
In case you want to modify the sources or even add your own externs, but don't know how to do the first steps, go to [this repository](https://github.com/cslev/p4extern/)


