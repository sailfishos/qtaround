#include <qtaround/os.hpp>

#include "tests_common.hpp"
#include <tut/tut.hpp>
#include <cor/util.hpp>
#include <cor/os.hpp>

#include <atomic>
#include <sys/types.h>
#include <sys/wait.h>

namespace os = qtaround::os;
namespace error = qtaround::error;
using error::dump;

namespace {

struct RootDir
{
    static const QString suffix;
public:

    RootDir(bool is_create = false)
        : name_(os::getTemp(suffix))
        , deleteIf_([]() { return true; })
    {
        if (is_create)
            create();
    }

    void deleteIf(std::function<bool()> cond)
    {
        deleteIf_ = cond;
    }

    bool create()
    {
        return os::mkdir(name_);
    }

    ~RootDir()
    {
        if (name_.right(suffix.size()) == suffix
            && os::path::isDir(name_)
            && deleteIf_())
            os::system("rm", {"-rf", name_});
    }

    QString operator()() const
    {
        return name_;
    }
private:
    QString name_;
    std::function<bool()> deleteIf_;
};

const QString RootDir::suffix = ".qtaround-test-f1f6f3868167b337dea6a229de1f3f4a";

}

namespace tut
{

struct os_test
{
    virtual ~os_test()
    {
    }
};

typedef test_group<os_test> tf;
typedef tf::object object;
tf qtaround_os_test("os");

enum test_ids {
    tid_basic =  1,
    tid_path,
    tid_path_checks,
    tid_mkdir,
    tid_mkTemp,
    tid_fileIO,
    tid_dirEntryOps,
    tid_fileInfo,
    tid_fileTime,
    tid_cp,
    tid_rename,
    tid_tree,
    tid_environ,
    tid_mountpoint,
    tid_stat,
    tid_diskFree,
    tid_du,
    tid_open_lock
};

#define DQ "\""
#define SQ "'"
#define QDQ SQ DQ SQ
#define QSQ DQ SQ DQ

template<> template<>
void object::test<tid_basic>()
{
    ensure_eq(AT, os::singleQuoted(""), "''");
    ensure_eq(AT, os::singleQuoted("a"), "'a'");
    ensure_eq(AT, os::singleQuoted("'"), QSQ);
    ensure_eq(AT, os::singleQuoted("''"), QSQ QSQ);
    ensure_eq(AT, os::singleQuoted("\"'"), QDQ QSQ);
    ensure_eq(AT, os::singleQuoted("'\""), QSQ QDQ);
    ensure_eq(AT, os::singleQuoted("a'"), "'a'" QSQ);
    ensure_eq(AT, os::singleQuoted("'b"), QSQ "'b'");
    ensure_eq(AT, os::singleQuoted("'c'"), QSQ "'c'" QSQ);
    ensure_eq(AT, os::singleQuoted("d'd"), "'d'" QSQ "'d'");
    ensure_eq(AT, os::singleQuoted("ee'"), "'ee'" QSQ);
    ensure_eq(AT, os::singleQuoted("'ff"), QSQ "'ff'");

    auto home = os::home();
    ensure_ne(AT, home.size(), 0);
    ensure_eq(AT, home, os::environ("HOME"));

    ensure_eq(AT, os::system("echo", {}), 0);
    QString test_cmd{"./subprocess_cmd_return_arg.sh"};

    auto res = os::system(test_cmd, {"0", "0"});
    ensure_eq(AT, res, 0);
    res = os::system(test_cmd, {"1", "0"});
    ensure_eq(AT, res, 1);
    res = os::system(test_cmd, {"0", "1"});
    ensure_eq(AT, res, 1);
    res = os::system(test_cmd, {"3", "5"});
    ensure_eq(AT, res, 8);
}

template<> template<>
void object::test<tid_path>()
{
    ensure_eq(AT, os::path::join(), "");
    ensure_eq(AT, os::path::join("", "/"), "/");
    ensure_eq(AT, os::path::join("/"), "/");
    ensure_eq(AT, os::path::join("/", "usr"), "/usr");
    ensure_eq(AT, os::path::join("/usr", "bin"), "/usr/bin");

    ensure_eq(AT, os::path::relative("/usr/bin", "/usr"), "bin");

    ensure_eq(AT, os::path::split("/"), strings("/"));
    ensure_eq(AT, os::path::split("//"), strings("/"));
    ensure_eq(AT, os::path::split("/usr//bin"), strings("/", "usr", "bin"));
    ensure_eq(AT, os::path::split("/usr//bin/"), strings("/", "usr", "bin"));
    ensure_eq(AT, os::path::split("/usr///bin/"), strings("/", "usr", "bin"));
    ensure_eq(AT, os::path::split("usr///bin"), strings("usr", "bin"));
}

template<> template<>
void object::test<tid_path_checks>()
{
    ensure(AT, !os::path::exists("non_existing_file"));
    ensure(AT, os::path::exists(os::home()));

    ensure(AT, os::path::isDir(os::home()));
    ensure(AT, !os::path::isDir("non_existing_file"));

    auto tmp = os::getTemp();
    ensure(AT, os::path::isDir(tmp));
    auto ln_tgt = os::path::join(tmp, "cutes_lib_test_file1");
    os::system("touch", {ln_tgt});
    auto rm_tgt = cor::on_scope_exit([ln_tgt]() { os::system("rm", {ln_tgt}); });
    ensure(AT, os::path::isFile(ln_tgt));
    ensure(AT, !os::path::isFile("non_existing_file"));

    auto ln = os::path::join(tmp, "cutes_lib_test_link1");
    os::system("ln" , {"-s", ln_tgt, ln});
    ensure("Should be link", os::path::isSymLink(ln));
    ensure_eq("", os::path::deref(ln), ln_tgt);
}

template<> template<>
void object::test<tid_mkdir>()
{
    bool is_ok = false;
    RootDir root;
    ensure("not root dir is expected to exist", !os::path::exists(root()));
    ensure("creating root for tests", root.create());
    ensure("root dir is created", os::path::exists(root()));
    ensure("root is dir", os::path::isDir(root()));
    root.deleteIf([&is_ok]() { return is_ok; });

    ensure("root for tests is created, expecting false", !root.create());

    auto dtree = os::path::join(root(), "w", "e");
    ensure(AT, !os::mkdir(dtree));
    ensure(AT, os::mkdir(dtree, {{"parent", true}}));
    ensure("dir with parent is created", os::path::exists(dtree));
    ensure(AT, os::path::isDir(dtree));
    is_ok = true;
}

template<> template<>
void object::test<tid_mkTemp>()
{
    auto name = os::mkTemp();
    ensure(AT, os::path::isFile(name));
    os::rm(name);
    name = os::mkTemp({{"dir", true}});
    ensure(AT, os::path::isDir(name));
    os::rmtree(name);
}

template<> template<>
void object::test<tid_fileIO>()
{
    auto data = "a\nb";
    RootDir root{true};
    auto fname = os::path::join(root(), "cutes_lib_test_aa");
    os::write_file(fname, data);
    ensure(AT, os::path::exists(fname));
    ensure(AT, os::path::isFile(fname));
    auto read = str(os::read_file(fname));
    ensure_eq(AT, read, data);
}

template<> template<>
void object::test<tid_dirEntryOps>()
{
    RootDir root{true};
    auto fname = os::path::join(root(), "cutes_lib_test_dirent_unlink");
    os::write_file(fname, "...");
    ensure(AT, os::path::exists(fname));
    os::unlink(fname);
    ensure(AT, !os::path::exists(fname));
}

template<> template<>
void object::test<tid_fileInfo>()
{
    auto dname = "path_info";
    RootDir root(true);
    auto d = os::path::join(root(), dname);
    ensure_eq(AT, os::path::fileName(d), dname);
    ensure_eq(AT, os::path::baseName(d), dname);
    ensure_eq(AT, os::path::dirName(d), root());
    ensure_eq(AT, os::path::suffix(d), "");
    ensure_eq(AT, os::path::completeSuffix(d), "");
    os::mkdir(d);
    auto ddot = os::path::join(d, ".");
    ensure_eq(AT, os::path::canonical(ddot), d);
    ensure(AT, os::path::isSelf(ddot));
    ensure(AT, !os::path::isSelf(d));
    auto ddots = os::path::join(d, "..", ".", dname);
    ensure_eq(AT, os::path::canonical(ddots), d);

    ensure(AT, os::path::isDescendent(d, root()));
    ensure(AT, !os::path::isDescendent(root(), d));
    ensure(AT, os::path::isDescendent(os::path::join(d, "..", dname), root()));

    auto base = str("test_file");
    auto suffix = str("e2");
    auto completeSuffix = str("e1.", suffix);
    auto name1 = str(base, ".", completeSuffix);
    auto f1 = os::path::join(d, name1);
    ensure_eq(AT, os::path::fileName(f1), name1);
    ensure_eq(AT, os::path::baseName(f1), base);
    ensure_eq(AT, os::path::suffix(f1), suffix);
    ensure_eq(AT, os::path::completeSuffix(f1), completeSuffix);
}

template<> template<>
void object::test<tid_fileTime>()
{
    RootDir root{true};
    auto fname = os::path::join(root(), "cutes_lib_test_time");
    os::write_file(fname, "1");
    auto t1 = os::lastModified(fname);
    usleep(100000 * 11); // sound be > 1s
    os::write_file(fname, "1");
    auto t2 = os::lastModified(fname);
    ensure_ne("Test precondition", t1.toString(), t2.toString());
    os::setLastModified(fname, t1);
    auto t3 = os::lastModified(fname);
    ensure_eq("Time is not set correctly?", t3.toString(), t1.toString());
}

template<> template<>
void object::test<tid_cp>()
{
    RootDir root{true};
    auto p1 = os::path::join(root(), "cp1");
    auto p2 = os::path::join(root(), "cp2");
    os::write_file(p1, "1");
    ensure_eq(AT, str(os::read_file(p1)), "1");
    auto rc = os::cp(p1, p2);
    ensure_eq(AT, rc, 0);
    ensure(AT, os::path::exists(p1));
    ensure(AT, os::path::exists(p2));
    ensure_eq(AT, str(os::read_file(p2)), "1");
    os::write_file(p2, "2");
    os::update(p1, p2);
    ensure_eq(AT, str(os::read_file(p2)), "2");
    auto p3 = os::path::join(root(), "cp3");
    os::update(p1, p3);
    ensure(AT, os::path::exists(p3));
    ensure_eq(AT, str(os::read_file(p3)), "1");
}

template<> template<>
void object::test<tid_rename>()
{
    RootDir root{true};
    auto p1 = os::path::join(root(), "rename1");
    auto p2 = os::path::join(root(), "rename2");
    os::write_file(p1, "1");
    ensure(AT, os::path::isFile(p1));
    os::rename(p1, p2);
    ensure(AT, os::path::isFile(p2));
    ensure(AT, !os::path::isFile(p1));
}

template<> template<>
void object::test<tid_tree>()
{
    RootDir root{true};
    auto treeRootSrc = os::path::join(root(), "treeTestSrc");
    os::mkdir(treeRootSrc);

    auto treeRootDst = os::path::join(root(), "treeTestDst");
    os::mkdir(treeRootSrc);

    auto f1 = os::path::join(treeRootSrc, "f1");
    auto f2 = os::path::join(treeRootSrc, "f2");
    os::write_file(f1, "1");
    os::write_file(f2, "2");

    os::cptree(os::path::join(treeRootSrc, "."), treeRootDst);
    ensure(AT, os::path::isFile(os::path::join(treeRootDst, "f1")));
    ensure(AT, os::path::isFile(os::path::join(treeRootDst, "f2")));

    os::symlink(f1, os::path::join(treeRootSrc, "link"));
    os::cptree(os::path::join(treeRootSrc, "."), treeRootDst);
    ensure(AT, os::path::isSymLink(os::path::join(treeRootDst, "link")));

    os::cptree(os::path::join(treeRootSrc, "."), treeRootDst, {{"deref", true}});
    ensure(AT, os::path::isFile(os::path::join(treeRootDst, "link")));
}

template<> template<>
void object::test<tid_environ>()
{
    auto env_home = os::environ("HOME");
    ensure_ne(AT, env_home, "");
    auto home = os::home();
    ensure_eq(AT, env_home, home);
}

template<> template<>
void object::test<tid_mountpoint>()
{
    auto home = os::home();
    auto mp = os::mountpoint(home);
    ensure_ne("There should be some mountpoint", mp.size(), 0);
    ensure(AT, os::path::isDescendent(home, mp));
    auto s = os::stat(home, {{"fields", "m"}});
    auto mp2 = s["mount_point"];
    if (mp != "?")
        ensure_eq("Should be the same mountpoint if it is not ?", mp2, mp);
}

template<> template<>
void object::test<tid_stat>()
{
    ensure_throws<error::Error>(AT, []() { os::stat(os::home()); });

    auto info = os::stat(os::home(), {{"fields", "ms"}});
    auto info_dump = [&info](char const *msg) {
        return str(msg) + ":" + dump(info);
    };
    ensure(AT, os::path::isDir(info["mount_point"]));
    ensure_ge(info_dump("Size is wrong"), info["size"].toUInt(), 1);
    info = os::stat(info["mount_point"], {{"filesystem", true}, {"fields", "abS"}});
    auto blocks = info["blocks"].toUInt()
        , bsize = info["block_size"].toUInt()
        , free_blocks = info["free_blocks_user"].toUInt();
    ensure_ge(info_dump("No blocks"), blocks, 1);
    ensure_ge(info_dump("Block size > 0"), bsize, 1);
    ensure_ge(info_dump("Free blocks > 0"), free_blocks, 1);
    ensure_ge(info_dump("Total >= Free"), blocks, free_blocks);
}

template<> template<>
void object::test<tid_diskFree>()
{
    auto res = os::diskFree(os::path::join(os::home()));
    ensure_ge("Most probably free space > 0", res, 1.0);

}

template<> template<>
void object::test<tid_du>()
{
    RootDir root{true};
    auto test_dir = os::path::join(root(), "du");
    os::mkdir(test_dir);
    os::system("dd", {"if=/dev/zero", "of=" + os::path::join(test_dir, "a")
                , "bs=1024", "count=2"});
    auto res = os::du(test_dir);
    ensure_ge("du should be a number > 0", res.toUInt(), 1);
    os::system("dd", {"if=/dev/zero", "of=" + os::path::join(test_dir, "b")
                , "bs=1024", "count=100"});
    res = os::du(test_dir);
    ensure_ge("du should be a number > 100", res.toUInt(), 100);

    auto dir_c = os::path::join(test_dir, "c");
    os::mkdir(dir_c);
    os::write_file(os::path::join(dir_c, "some_file"), "some_data");
    res = os::du(test_dir, {{"summarize", false}});
    ensure(AT, hasType(res, QMetaType::QVariantMap));
    auto items = res.toMap();
    ensure_eq(AT, items.size(), 2);
    for (auto it = items.begin(); it != items.end(); ++it) {
        ensure(AT, os::path::isDescendent(it.key(), test_dir));
        ensure_ge(AT, it.value().toUInt(), 1);
    }
}

template<> template<>
void object::test<tid_open_lock>()
{
    static const auto name = os::getTemp("qtaround-test-lock");

    // non-reenterable
    static std::atomic<char> poll_err_count(ATOMIC_VAR_INIT(0));

    using cor::posix::Pipe;
    namespace os = qtaround::os;

    // parent sends cookie to the forked child and expects incremented
    // cookie back
    auto to_child = Pipe::create(), from_child = Pipe::create();

    auto child = [&]() {
        auto in = cor::use<Pipe::Read>(to_child);
        auto out = cor::use<Pipe::Write>(from_child);
        int cookie = 5;

        auto pollIn = [&in, &cookie]() {
            auto ev = cor::poll(in, POLLIN | POLLPRI, 1000 * 5);
            if ((ev & (POLLHUP | POLLERR)) || !ev) {
                ++poll_err_count;
                return false;
            }
            return cor::read(in, cookie) == sizeof(cookie);
        };
        auto write = [&out, &cookie]() {
            return cor::write(out, cookie) == sizeof(cookie);
        };

        auto shouldNotLock = [&cookie]() {
            bool is_locked = false;
            auto got_unexpected_lock = [&is_locked](os::FileLock) {
                is_locked = true;
            };
            auto res = os::tryLock(name, got_unexpected_lock);
            if (!(!res || is_locked))
                ++cookie;
            return true;
        };

        auto shouldLock = [&cookie, &write, &pollIn]() {
            bool is_locked = false;
            auto got_expected_lock = [&](os::FileLock) {
                is_locked = true;
                ++cookie;
                write();
                // locked till parent allow to go
                pollIn();
            };
            auto res = os::tryLock(name, got_expected_lock);
            // if not locked parent should be notified /w the same cookie
            if (!is_locked) {
                write();
                pollIn();
            }
            if (!res) ++cookie;
            write();
        };

        if (pollIn()) shouldNotLock();
        write();
        if (pollIn()) shouldLock();
    };

    auto parent = [&]() {
        auto in = cor::use<Pipe::Read>(from_child);
        auto out = cor::use<Pipe::Write>(to_child);

        auto delOnExit = cor::on_scope_exit([]() {
                if (os::path::exists(name)) os::rm(name);
            });

        auto pollRead = [&in](std::string const &msg, int data) {
            auto events = cor::poll(in, POLLIN | POLLPRI, 1000 * 5);
            ensure_eq("Bad poll event:" + msg, events & (POLLHUP | POLLERR), 0);
            ensure_ne("No reply:" + msg, events, 0);
            int res = 0;
            ensure_eq("Read:" + msg, cor::read(in, res), sizeof(res));
            ensure_eq(msg, res, data);
            return res;
        };

        auto lockChildCantLock = [&]() {
            auto isLocked = false;
            auto gotExpectedLock = [&isLocked, &out, &pollRead](os::FileLock) {
                isLocked = true;
                cor::write(out, 1);
                pollRead("Child should not be able to lock", 2);
            };
            auto res = os::tryLock(name, gotExpectedLock);
            ensure("Should get lock", isLocked);
            ensure("Should get null locker", !res);
        };

        auto childLockParentLockLater = [&]() {
            cor::write(out, 11);
            pollRead("Child should be able to lock", 12);
            auto shouldNotLock = []() {
                auto gotUnexpectedLock = [](os::FileLock) {
                    fail("Parent should not be able to lock");
                };
                auto locker = os::tryLock(name, gotUnexpectedLock);
                ensure("Parent should get locker", !!locker);
                return std::move(locker);
            };
            auto locker = shouldNotLock();
            cor::write(out, 21);
            pollRead("Child should release lock, child locker should be null", 22);
            ensure("Locker should be available", !!locker);

            auto isLocked = false;
            auto gotExpectedLock = [&isLocked](os::FileLock) {
                isLocked = true;
            };
            locker = os::tryLock(std::move(locker), gotExpectedLock);
            ensure("Should get lock", isLocked);
            ensure("Should get null locker", !locker);
        };

        lockChildCantLock();
        childLockParentLockLater();
    };

    auto rc = ::fork();
    ensure_ne("Can't fork", rc, -1);

    auto wait_on_exit = [] () {
        int child_status = 255;
        int counter = 1000 * 5;
        int rc;
        while (!(rc = ::waitpid(-1, &child_status, WNOHANG))) {
            if (rc == -1) return 100;
            if (!--counter) return 101;
            ::usleep(1000);
        }

        return child_status;
    };

    if (!rc)
        child();
    else {
        try {
            parent();
        } catch (...) {
            wait_on_exit();
            throw;
        }
        ensure_eq("Bad child", wait_on_exit(), 0);
    }
}

}
