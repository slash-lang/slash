<%

class Struct {
    def self.new(attrs) {
        throw TypeError.new("Attribute names must be Strings") unless attrs.all(String:has_instance);

        klass = Class.new;

        klass.define_method('init, \opts {
            for k => v in opts {
                if attrs.includes(k) {
                    set_instance_variable(k, v);
                } else {
                    throw ArgumentError.new("No such attribute '#{k}'");
                }
            }
        });

        klass.singleton_class.define_method('attributes, \{ attrs });
        klass.define_method('attributes, \{ attrs });

        attrs.each(\k {
            klass.define_method(k, \{
                get_instance_variable(k);
            });
            klass.define_method(k + "=", \(value){
                set_instance_variable(k, value);
            });
        });

        klass;
    }
}

