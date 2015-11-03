#include <qtaround/debug.hpp>
#include <tut/tut.hpp>
#include <cor/pipe.hpp>
#include "tests_common.hpp"

#include <vector>

namespace debug = qtaround::debug;

namespace tut
{

struct debug_test
{
    virtual ~debug_test()
    {
    }
};

typedef test_group<debug_test> tf;
typedef tf::object object;
tf vault_debug_test("debug");

enum test_ids {
    tid_levels =  1
};

template<> template<>
void object::test<tid_levels>()
{
    using debug::Level;
    cor::Pipe pipe;
    std::vector<Level> levels = {{
            Level::Debug, Level::Info, Level::Warning
            , Level::Error, Level::Critical
        }};
    for (auto lvl : levels) {
        debug::level(lvl);
        debug::debug(1);
        debug::info(2);
        debug::warning(3);
        debug::error(4);
        debug::critical(5);
    }
    
    debug::debug(1, "a");
    for (auto lvl : levels) {
        debug::level(lvl);
        debug::debug("a", 1);
        debug::info("b", 2);
        debug::warning("c", 3);
        debug::error("d", 4);
        debug::critical("e", 5);
    }
}

}
