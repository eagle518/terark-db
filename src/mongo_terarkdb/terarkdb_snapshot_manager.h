/**
 *    Copyright (C) 2016 Terark Inc.
 *    This file is heavily modified based on MongoDB WiredTiger StorageEngine
 *    Created on: 2015-12-01
 *    Author    : leipeng, rockeet@gmail.com
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <boost/optional.hpp>
#include "mongo_terarkdb_common.hpp"

#include "mongo/base/disallow_copying.h"
#include "mongo/db/storage/snapshot_manager.h"
#include "terarkdb_util.h"
#include "mongo/stdx/mutex.h"

namespace mongo { namespace terarkdb {

class TerarkDbSnapshotManager final : public SnapshotManager {
    MONGO_DISALLOW_COPYING(TerarkDbSnapshotManager);

public:
    explicit TerarkDbSnapshotManager(TerarkDb_CONNECTION* conn) {
        invariantTerarkDbOK(conn->open_session(conn, NULL, NULL, &_session));
    }

    ~TerarkDbSnapshotManager() {
        shutdown();
    }

    Status prepareForCreateSnapshot(OperationContext* txn) final;
    Status createSnapshot(OperationContext* ru, const SnapshotName& name) final;
    void setCommittedSnapshot(const SnapshotName& name) final;
    void cleanupUnneededSnapshots() final;
    void dropAllSnapshots() final;

    //
    // TerarkDb-specific methods
    //

    /**
     * Prepares for a shutdown of the TerarkDb_CONNECTION.
     */
    void shutdown();

    /**
     * Starts a transaction and returns the SnapshotName used.
     *
     * Throws if there is currently no committed snapshot.
     */
    SnapshotName beginTransactionOnCommittedSnapshot(TerarkDb_SESSION* session) const;

    /**
     * Returns lowest SnapshotName that could possibly be used by a future call to
     * beginTransactionOnCommittedSnapshot, or boost::none if there is currently no committed
     * snapshot.
     *
     * This should not be used for starting a transaction on this SnapshotName since the named
     * snapshot may be deleted by the time you start the transaction.
     */
    boost::optional<SnapshotName> getMinSnapshotForNextCommittedRead() const;

private:
    mutable stdx::mutex _mutex;  // Guards all members.
    boost::optional<SnapshotName> _committedSnapshot;
    TerarkDb_SESSION* _session;  // only used for dropping snapshots.
};
} }  // namespace mongo::terark

