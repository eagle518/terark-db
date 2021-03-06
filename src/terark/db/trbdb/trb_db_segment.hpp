#pragma once

#include <mutex>
#include <terark/io/FileStream.hpp>
#include <terark/io/StreamBuffer.hpp>
#include <terark/io/DataIO.hpp>
#include <terark/db/db_segment.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <tbb/spin_mutex.h>
#include <tbb/queuing_rw_mutex.h>
#include <atomic>
#undef min
#undef max
#include <terark/threaded_rbtree_hash.h>


namespace terark { namespace db { namespace trbdb {

class TERARK_DB_DLL RowLockTransaction;
class TrbLogger;

struct TrbReadDeletedRecordException
{
    llong id;
};

struct TrbRWRowMutex : boost::noncopyable
{
private:
    typedef tbb::queuing_rw_mutex rw_lock_t;
    typedef tbb::spin_mutex spin_lock_t;

    struct map_item
    {
        map_item(uint32_t);
        uint32_t id;
        uint32_t count;
        rw_lock_t lock;
    };
    template<size_t Size, class Unused>
    struct hasher
    {
        size_t operator()(llong id) const
        {
            return size_t(id);
        }
    };
    template<class Unused>
    struct hasher<4, Unused>
    {
        size_t operator()(llong id) const
        {
            return size_t(id & 0xFFFFFFFF) | size_t(uint64_t(id) >> 32);
        }
    };
    trb_hash_map<uint32_t, map_item *, hasher<sizeof(size_t), void>> row_lock;
    valvec<map_item *> lock_pool;
    spin_lock_t g_lock;

public:
    ~TrbRWRowMutex();

    class scoped_lock
    {
    private:
        TrbRWRowMutex *parent;
        map_item *item;
        rw_lock_t::scoped_lock lock;

    public:
        scoped_lock(TrbRWRowMutex &mutex, size_t id, bool write = true);
        ~scoped_lock();

        bool upgrade();
        bool downgrade();
    };
};

class TERARK_DB_DLL TrbColgroupSegment : public ColgroupWritableSegment {

protected:
    friend class RowLockTransaction;
    mutable TrbRWRowMutex m_rowMutex;
    mutable TrbLogger *m_logger;

public:
	class TrbDbTransaction; friend class TrbDbTransaction;
	DbTransaction* createTransaction(DbContext*) override;

    TrbColgroupSegment();
	~TrbColgroupSegment();

    void load(PathRef path) override;
    void save(PathRef path) const override;

protected:
    void initEmptySegment() override;

    ReadableIndex *openIndex(const Schema &, PathRef segDir) const override;
    ReadableIndex *createIndex(const Schema &, PathRef segDir) const override;
    ReadableStore *createStore(const Schema &, PathRef segDir) const override;

public:
    void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;

    void selectColumns(llong recId, const size_t* colsId, size_t colsNum,
                       valvec<byte>* colsData, DbContext*) const override;
    void selectOneColumn(llong recId, size_t columnId,
                         valvec<byte>* colsData, DbContext*) const override;

    void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize,
                         valvec<byte>* cgDataVec, DbContext*) const override;

    llong append(fstring, DbContext *) override;
    void remove(llong, DbContext *) override;
    void update(llong, fstring, DbContext *) override;

    void shrinkToFit(void) override;

    void saveRecordStore(PathRef segDir) const override;
    void loadRecordStore(PathRef segDir) override;

    llong dataStorageSize() const override;
    llong totalStorageSize() const override;
};

}}} // namespace terark::db::wt
