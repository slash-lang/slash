<%

use Struct;

class StructTest extends Test {
    def test_attributes {
        struct = Struct.new([]);
        assert_equal([], struct.attributes);
        assert_equal([], struct.new({}).attributes);

        struct = Struct.new(['a, 'b, 'c]);
        assert_equal(['a, 'b, 'c], struct.attributes);
        assert_equal(['a, 'b, 'c], struct.new({}).attributes);
    }

    def test_new {
        assert_is_a(Class, Struct.new([]));
        assert_throws(TypeError, \{ Struct.new([1]); });
    }

    def test_struct_attributes {
        person = Struct.new(['name, 'age]);

        charlie = person.new({ 'name => "Charlie", 'age => 18 });
        assert_equal("Charlie", charlie.name);
        assert_equal(18, charlie.age);

        charlie = person.new({ 'name => "Charlie" });
        assert_equal("Charlie", charlie.name);
        assert_equal(nil, charlie.age);
    }

    def test_struct_doesn't_allow_extra_attributes_in_init {
        person = Struct.new(['name, 'age]);

        assert_throws(ArgumentError, \{
            person.new({ 'name => "Charlie", 'nope => nil });
            print("\nwhatever\n");
        });
    }

    def test_can_assign_attributes {
        person = Struct.new(['name, 'age]);
        charlie = person.new({});

        assert_equal(nil, charlie.name);
        assert_equal(nil, charlie.age);

        charlie.name = "Charlie";
        charlie.age = 18;

        assert_equal("Charlie", charlie.name);
        assert_equal(18, charlie.age);
    }
}.register;
