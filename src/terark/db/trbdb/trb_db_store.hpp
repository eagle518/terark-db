#pragma once

#include <terark/db/db_table.hpp>
#include <terark/util/fstrvec.hpp>
#include <set>
#include <tbb/mutex.h>
#include <terark/mempool.hpp>
#include <terark/io/var_int.hpp>

namespace terark { namespace db { namespace trbdb {

class TrbStoreIterForward;
class TrbStoreIterBackward;

class TERARK_DB_DLL TrbWritableStore : public ReadableStore, public WritableStore {
protected:
    typedef std::size_t size_type;
    typedef terark::MemPool<4> pool_type;
    struct data_object
    {
        byte data[1];
    };
    valvec<uint32_t> m_index;
    pool_type m_data;

    fstring readItem(size_type i) const;
    void storeItem(size_type i, fstring d);
    bool removeItem(size_type i);

    friend class TrbStoreIterForward;
    friend class TrbStoreIterBackward;

public:
    TrbWritableStore();
	~TrbWritableStore();

	void save(PathRef) const override;
	void load(PathRef) override;

	llong dataStorageSize() const override;
	llong dataInflateSize() const override;
	llong numDataRows() const override;
	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;

	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;

	llong append(fstring row, DbContext*) override;
	void  update(llong id, fstring row, DbContext*) override;
	void  remove(llong id, DbContext*) override;

	void shrinkToFit() override;

	AppendableStore* getAppendableStore() override;
	UpdatableStore* getUpdatableStore() override;
	WritableStore* getWritableStore() override;
};
typedef boost::intrusive_ptr<TrbWritableStore> TrbWritableStorePtr;

}}} // namespace terark::db::wt