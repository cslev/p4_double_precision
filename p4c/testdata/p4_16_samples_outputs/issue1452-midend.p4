control c() {
    bit<32> x;
    @name("c.a") action a_0() {
        x = 32w1;
    }
    @hidden table tbl_a {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_a.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;

