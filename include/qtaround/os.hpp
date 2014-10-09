#ifndef _CUTES_OS_HPP_
#define _CUTES_OS_HPP_
/**
 * @file os.hpp
 * @brief Wrappers to support API looking familiar for python/shell developers
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include "error.hpp"
#include "sys.hpp"
#include "subprocess.hpp"

#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QLockFile>

#include <memory>


namespace qtaround { namespace os {

QString singleQuoted(QString const &);

using subprocess::Process;

namespace path {

inline QString join()
{
    return QString("");
}

inline QString join(QString const& a)
{
    return a;
}

// to be used by templates below
QString join(QStringList);

template <typename ...A>
QString join(QStringList data, QString const& a , A&& ...args)
{
    return join(data << a, std::forward<A>(args)...);
}

template <typename ...A>
QString join(QString const& a, QString const& b, A&& ...args)
{
    return join(QStringList{a, b}, std::forward<A>(args)...);
    //return join(QStringList({a, b}).join("/"), std::forward<A>(args)...);
}

inline QString join(std::initializer_list<QString> data)
{
    return join(QStringList(data));
}

inline bool exists(QString const &p)
{
    return QFileInfo(p).exists();
}
inline QString canonical(QString const &p)
{
    return QFileInfo(p).canonicalFilePath();
}
inline QString relative(QString const &p, QString const &d)
{
    return QDir(d).relativeFilePath(p);
}
inline QString deref(QString const &p)
{
    return QFileInfo(p).symLinkTarget();
}
QString target(QString const &link);

inline bool isDir(QString const &p)
{
    return QFileInfo(p).isDir();
}
inline bool isSymLink(QString const &p)
{
    return QFileInfo(p).isSymLink();
}
inline bool isFile(QString const &p)
{
    return QFileInfo(p).isFile();
}
inline QString dirName(QString const &p)
{
    return QFileInfo(p).dir().path();
}
inline QString fileName(QString const &p)
{
    return QFileInfo(p).fileName();
}
inline QString baseName(QString const &p)
{
    return QFileInfo(p).baseName();
}
inline QString suffix(QString const &p)
{
    return QFileInfo(p).suffix();
}
inline QString completeSuffix(QString const &p)
{
    return QFileInfo(p).completeSuffix();
}
// applicable to existing paths only
inline bool isSame(QString const &p1, QString const &p2)
{
    return canonical(p1) == canonical(p2);
};
// applicable to existing paths only
inline bool isSelf(QString const &p)
{
    return isSame(dirName(p), p);
};

QStringList split(QString const &p);

bool isDescendent(QString const &p, QString const &other);

} // path

int system(QString const &cmd, QStringList const &args = QStringList());

bool mkdir(QString const &path, QVariantMap const &options);

static inline bool mkdir(QString const &path)
{
    return mkdir(path, QVariantMap());
}

QByteArray read_file(QString const &fname);
ssize_t write_file(QString const &fname, QByteArray const &data);

static inline ssize_t write_file(QString const &fname, QString const &data)
{
    return write_file(fname, data.toUtf8());
}

static inline ssize_t write_file(QString const &fname, char const *data)
{
    return write_file(fname, QString(data).toUtf8());
}

namespace {

inline QString home()
{
    return QDir::homePath();
}

inline int symlink(QString const &tgt, QString const &link)
{
    return system("ln", {"-s", tgt, link});
}

inline int rmtree(QString const &path)
{
    return system("rm", {"-rf", path});
}

inline int unlink(QString const &what)
{
    return system("unlink", {what});
}

inline int rename(QString const &from, QString const &to)
{
    return system("mv", {from, to});
}

inline QDateTime lastModified(QString const &path)
{
    return QFileInfo(path).lastModified();
}

inline int setLastModified(QString const &path, QDateTime const &timeval)
{
    return system("touch", {"-d", timeval.toString(), path});
};

inline int rm(QString const &path)
{
    return system("rm", {path});
}

inline QString environ(QString const &name)
{
    auto c = ::getenv(name.toUtf8());
    return c ? str(c) : QString();
}

}

int cp(QString const &, QString const &, QVariantMap &&);

static inline int cp(QString const &src, QString const &dst)
{
    return cp(src, dst, QVariantMap());
}

int update(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());

int update_tree(QString const &, QString const &, QVariantMap &&);

static inline int update_tree(QString const &src, QString const &dst)
{
    return update_tree(src, dst, QVariantMap());
}

int cptree(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());

size_t get_block_size(QString const &);

QList<QVariantMap> mount();
QString mountpoint(QString const &path);
string_map_type stat(QString const &path, QVariantMap &&options = QVariantMap());
QVariant du(QString const &path, QVariantMap &&options = map({{"summarize", true}
            , {"one_filesystem", true}, {"block_size", "K"}}));
double diskFree(QString const &path);
QString mkTemp(QVariantMap &&options = QVariantMap());

static inline QString getTemp()
{
    return QDir::tempPath();
}

template <typename ...A>
static inline QString getTemp(A &&...args)
{
    return os::path::join(getTemp(), std::forward<A>(args)...);
}

typedef std::unique_ptr<QLockFile> LockFileHandle;
class FileLock
{
public:
    FileLock(LockFileHandle &&from) : lock_(std::move(from)) {}
    //lock_ is movable only
private:
    LockFileHandle lock_;
};

template <typename OnLocked>
LockFileHandle tryLock(LockFileHandle &&locker, OnLocked fn)
{
    if (locker) {
        if (locker->tryLock()) {
            fn(FileLock(std::move(locker)));
            return nullptr;
        }
    }
    return std::move(locker);
}

template <typename OnLocked>
LockFileHandle tryLock(QString const &fileName, OnLocked fn)
{
    LockFileHandle h{new QLockFile(fileName)};
    return tryLock(std::move(h), fn);
}

}}

#ifdef QTAROUND_NO_NS
namespace os = qtaround::os;
#endif

#endif // _CUTES_OS_HPP_
