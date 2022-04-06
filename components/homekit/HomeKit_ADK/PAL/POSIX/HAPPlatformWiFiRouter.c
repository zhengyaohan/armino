// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include "HAPPlatformWiFiRouter+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "WiFiRouter" };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandleSQLiteLog(void* _Nullable context, int errCode, const char* message) {
    HAPAssert(!context);

    HAPLogError(&logObject, "SQLite: %s (%d) - %s", sqlite3_errstr(errCode), errCode, message);
}

/**
 * Configures and initializes SQLite.
 */
static void SetupSQLite() {
    int errCode;

    // Check SQLite consistency.
    HAPAssert(sqlite3_libversion_number() == SQLITE_VERSION_NUMBER);
    HAPLog(&logObject,
           "SQLite version: %s (%d) - %s",
           sqlite3_libversion(),
           sqlite3_libversion_number(),
           sqlite3_sourceid());

    // Configure SQLite.
    // See https://www.sqlite.org/c3ref/c_config_covering_index_scan.html
    errCode = sqlite3_config(SQLITE_CONFIG_LOG, HandleSQLiteLog, NULL);
    HAPAssert(!errCode);

    // Initialize SQLite.
    errCode = sqlite3_initialize();
    HAPAssert(!errCode);
}

//----------------------------------------------------------------------------------------------------------------------

static int HandleSQLiteTrace(unsigned int traceCode, void* _Nullable context, void* p, void* x) {
    HAPPrecondition(!context);

    switch (traceCode) {
        case SQLITE_TRACE_STMT: {
            sqlite3_stmt* stmt = p;
            const char* message = x;
            if (message && HAPStringHasPrefix(&message[0], "--")) {
                HAPLogDebug(&logObject, "SQLite: %p\n%s", (const void*) stmt, message);
            } else {
                char truncatedMessage[2000];
                char* sql = sqlite3_expanded_sql(stmt);
                size_t numSQLBytes = HAPMin(HAPStringGetNumBytes(sql), sizeof truncatedMessage - 1);
                HAPRawBufferCopyBytes(truncatedMessage, sql, numSQLBytes);
                truncatedMessage[numSQLBytes] = '\0';
                HAPLogDebug(&logObject, "SQLite: %p\n%s", (const void*) stmt, truncatedMessage);
                sqlite3_free_safe(sql);
            }
        }
            return 0;
        case SQLITE_TRACE_PROFILE: {
            sqlite3_stmt* stmt = p;
            int64_t* elapsedNs = x;
            HAPLogDebug(
                    &logObject,
                    "SQLite: %p - Ran for %lld.%06lld ms.",
                    (const void*) stmt,
                    (long long) (*elapsedNs / (1000 * 1000)),
                    (long long) (*elapsedNs % (1000 * 1000)));
        }
            return 0;
        case SQLITE_TRACE_ROW: {
            sqlite3_stmt* stmt = p;
            HAPLogDebug(&logObject, "SQLite: %p - Row returned.", (const void*) stmt);
        }
            return 0;
        case SQLITE_TRACE_CLOSE: {
            sqlite3* db = p;
            HAPLogInfo(&logObject, "SQLite: %p - Database closed.", (const void*) db);
        }
            return 0;
        default: {
            HAPLogError(&logObject, "Unknown trace event code: %u.", traceCode);
        }
            HAPFatalError();
    }
}

/**
 * Opens the database file at the specified path.
 *
 * @param[out] db                   Database connection.
 * @param      dbFile               Database file location. NULL to use in-memory storage.
 */
static void ConnectDatabase(sqlite3** db, const char* _Nullable dbFile) {
    HAPPrecondition(db);

    int errCode;

    if (!dbFile) {
        dbFile = ":memory:";
    }
    HAPLogInfo(&logObject, "Connecting database: %s", dbFile);
    errCode = sqlite3_open(dbFile, db);
    HAPAssert(!errCode);
    HAPAssert(*db);
    HAPLogInfo(&logObject, "SQLite: %p - Database opened.", (const void*) *db);

    errCode = sqlite3_trace_v2(
            *db,
            SQLITE_TRACE_STMT |
                    //        SQLITE_TRACE_PROFILE |
                    //        SQLITE_TRACE_ROW |
                    SQLITE_TRACE_CLOSE,
            HandleSQLiteTrace,
            NULL);
    HAPAssert(!errCode);

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            *db,
            "PRAGMA user_version",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    HAPAssert(!errCode);
    errCode = sqlite3_step(stmt);
    HAPAssert(errCode == SQLITE_ROW);
    sqlite_int64 userVersion = sqlite3_column_int64(stmt, /* iCol: */ 0);
    sqlite3_finalize_safe(stmt);
    HAPLogInfo(&logObject, "SQLite: %p - Schema version %llu.", (const void*) *db, (unsigned long long) userVersion);
    if (userVersion > 1) {
        HAPLogError(
                &logObject,
                "SQLite: %p - Downgrade from schema version %llu is not supported.",
                (const void*) *db,
                (unsigned long long) userVersion);
        HAPFatalError();
    }

    // See https://www.sqlite.org/pragma.html
    errCode = sqlite3_exec(
            *db,
            "PRAGMA application_id = 0x48415021;\n" // 'H' 'A' 'P' kHAPAccessoryCategory_WiFiRouters
            "PRAGMA auto_vacuum = FULL;\n"
            "PRAGMA automatic_index = ON;\n"
            "PRAGMA busy_timeout = 1000;\n"
            "PRAGMA encoding = 'UTF-8';\n"
            "PRAGMA foreign_keys = ON;\n"
            "PRAGMA fullfsync = ON;\n"
            "PRAGMA journal_mode = DELETE;\n"
            "PRAGMA secure_delete = ON;\n"
            "PRAGMA synchronous = EXTRA;\n"
            "PRAGMA user_version = 1;",
            NULL,
            NULL,
            NULL);
    HAPAssert(!errCode);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Creates the database schema.
 *
 * @param      db                   Database connection.
 */
static void CreateSchema(sqlite3* db) {
    HAPPrecondition(db);

    int errCode;

    // WANs.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_wans` (\n"
            "    `id` INTEGER PRIMARY KEY\n"
            "        CHECK(`id` > 0 AND `id` <= 4294967295),\n"
            "    `type` UNSIGNED TINYINT\n"
            "        -- HAPPlatformWiFiRouterWANType\n"
            "        CHECK(`type` >= 1 AND `type` <= 3)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client groups.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_groups` (\n"
            "    `id` INTEGER PRIMARY KEY\n"
            "        CHECK(`id` > 0 AND `id` <= 4294967295)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profiles.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_clients` (\n"
            "    `id` INTEGER PRIMARY KEY AUTOINCREMENT\n"
            "        CHECK(`id` > 0 AND `id` <= 4294967295),\n"
            "    `credential_mac` BINARY(6) UNIQUE\n"
            "        CHECK(LENGTH(`credential_mac`) = 6),\n"
            "    `credential_passphrase` VARCHAR(63) UNIQUE\n"
            "        CHECK(LENGTH(`credential_passphrase`) >= 8 AND LENGTH(`credential_passphrase`) <= 63),\n"
            "    `credential_psk` BINARY(32) UNIQUE\n"
            "        CHECK(LENGTH(`credential_psk`) = 32),\n"
            "    `wan_full_access` BOOL NOT NULL DEFAULT FALSE,\n"
            "    `lan_full_access` BOOL NOT NULL DEFAULT FALSE,\n"
            "    `last_reset_timestamp` UNSIGNED BIGINT\n"
            "        CHECK(`last_reset_timestamp` >= 0),\n"
            "    `last_violation_timestamp` UNSIGNED BIGINT\n"
            "        CHECK(`last_violation_timestamp` >= 0),\n"
            "    CHECK(\n"
            "        (`credential_mac` IS NOT NULL) +\n"
            "        (`credential_passphrase` IS NOT NULL) +\n"
            "        (`credential_psk` IS NOT NULL) = 1),\n"
            "    CHECK(`last_reset_timestamp` < `last_violation_timestamp`)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile groups.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_groups` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `group` INTEGER NOT NULL\n"
            "        REFERENCES `hap_groups` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    UNIQUE(`client`, `group`)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile Port WAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_wan_port` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `transport_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterTransportProtocol\n"
            "        CHECK(`transport_protocol` >= 1 AND `transport_protocol` <= 2),\n"
            "    `host_dns_name` TEXT,\n"
            "    `host_ip_start` VARBINARY(16)\n"
            "        CHECK(LENGTH(`host_ip_start`) = 4 OR LENGTH(`host_ip_start`) = 16),\n"
            "    `host_ip_end` VARBINARY(16)\n"
            "        CHECK(LENGTH(`host_ip_end`) = 4 OR LENGTH(`host_ip_end`) = 16),\n"
            "    `host_port_start` UNSIGNED SMALLINT NOT NULL\n"
            "        CHECK(`host_port_start` >= 0 AND `host_port_start` <= 65535),\n"
            "    `host_port_end` UNSIGNED SMALLINT\n"
            "        CHECK(`host_port_end` >= 0 AND `host_port_end` <= 65535),\n"
            "    CHECK(`host_ip_start` IS NOT NULL OR `host_ip_end` IS NULL),\n"
            "    CHECK(LENGTH(`host_ip_start`) = LENGTH(`host_ip_end`)),\n"
            "    CHECK(`host_ip_end` IS NULL OR `host_ip_start` != X'00000000'),\n"
            "    CHECK(`host_ip_end` IS NULL OR `host_ip_start` != X'00000000000000000000000000000000'),\n"
            "    CHECK(`host_ip_start` < `host_ip_end`),\n"
            "    CHECK(`host_port_end` IS NULL OR `host_port_start` != 0),\n"
            "    CHECK(`host_port_start` < `host_port_end`)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile ICMP WAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_wan_icmp` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `host_dns_name` TEXT,\n"
            "    `host_ip_start` VARBINARY(16)\n"
            "        CHECK(LENGTH(`host_ip_start`) = 4 OR LENGTH(`host_ip_start`) = 16),\n"
            "    `host_ip_end` VARBINARY(16)\n"
            "        CHECK(LENGTH(`host_ip_end`) = 4 OR LENGTH(`host_ip_end`) = 16),\n"
            "    `icmp_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterICMPType\n"
            "        CHECK(`icmp_protocol` >= 1 AND `icmp_protocol` <= 2),\n"
            "    `icmp_type` UNSIGNED TINYINT\n"
            "        CHECK(`icmp_type` >= 0 AND `icmp_type` <= 255),\n"
            "    CHECK(`host_ip_start` IS NOT NULL OR `host_ip_end` IS NULL),\n"
            "    CHECK(LENGTH(`host_ip_start`) = LENGTH(`host_ip_end`)),\n"
            "    CHECK(`host_ip_end` IS NULL OR `host_ip_start` != X'00000000'),\n"
            "    CHECK(`host_ip_end` IS NULL OR `host_ip_start` != X'00000000000000000000000000000000'),\n"
            "    CHECK(`host_ip_start` < `host_ip_end`)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile Multicast Bridging LAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_lan_multicast_bridging` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `direction` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterFirewallRuleDirection\n"
            "        CHECK(`direction` >= 1 AND `direction` <= 2),\n"
            "    `peer_group` UNSIGNED INT\n"
            "        CHECK(`peer_group` > 0 AND `peer_group` <= 4294967295),\n"
            "    `destination_ip` VARBINARY(16) NOT NULL\n"
            "        CHECK(LENGTH(`destination_ip`) = 4 OR LENGTH(`destination_ip`) = 16),\n"
            "    `destination_port` UNSIGNED SMALLINT NOT NULL\n"
            "        CHECK(`destination_port` >= 0 AND `destination_port` <= 65535)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile Static Port LAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_lan_static_port` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `direction` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterFirewallRuleDirection\n"
            "        CHECK(`direction` >= 1 AND `direction` <= 2),\n"
            "    `transport_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterTransportProtocol\n"
            "        CHECK(`transport_protocol` >= 1 AND `transport_protocol` <= 2),\n"
            "    `peer_group` UNSIGNED INT\n"
            "        CHECK(`peer_group` > 0 AND `peer_group` <= 4294967295),\n"
            "    `destination_port_start` UNSIGNED SMALLINT NOT NULL\n"
            "        CHECK(`destination_port_start` >= 0 AND `destination_port_start` <= 65535),\n"
            "    `destination_port_end` UNSIGNED SMALLINT\n"
            "        CHECK(`destination_port_end` >= 0 AND `destination_port_end` <= 65535),\n"
            "    CHECK(`destination_port_end` IS NULL OR `destination_port_start` != 0),\n"
            "    CHECK(`destination_port_start` < `destination_port_end`)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile Dynamic Port LAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_lan_dynamic_port` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `direction` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterFirewallRuleDirection\n"
            "        CHECK(`direction` >= 1 AND `direction` <= 2),\n"
            "    `transport_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterTransportProtocol\n"
            "        CHECK(`transport_protocol` >= 1 AND `transport_protocol` <= 2),\n"
            "    `peer_group` UNSIGNED INT\n"
            "        CHECK(`peer_group` > 0 AND `peer_group` <= 4294967295),\n"
            "    `advertisement_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol\n"
            "        CHECK(`advertisement_protocol` >= 1 AND `advertisement_protocol` <= 2),\n"
            "    `service_type` TEXT,\n"
            "    `advertisement_only` BOOL NOT NULL\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);

    // Network client profile Static ICMP LAN firewall rules.
    errCode = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS `hap_client_firewalls_lan_static_icmp` (\n"
            "    `client` INTEGER NOT NULL\n"
            "        REFERENCES `hap_clients` ON DELETE CASCADE ON UPDATE CASCADE,\n"
            "    `sort_order` UNSIGNED INTEGER NOT NULL\n"
            "        CHECK(`sort_order` >= 0 AND `sort_order` <= 4294967295),\n"
            "    `direction` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterFirewallRuleDirection\n"
            "        CHECK(`direction` >= 1 AND `direction` <= 2),\n"
            "    `peer_group` UNSIGNED INT\n"
            "        CHECK(`peer_group` > 0 AND `peer_group` <= 4294967295),\n"
            "    `icmp_protocol` UNSIGNED TINYINT NOT NULL\n"
            "        -- HAPPlatformWiFiRouterICMPType\n"
            "        CHECK(`icmp_protocol` >= 1 AND `icmp_protocol` <= 2),\n"
            "    `icmp_type` UNSIGNED TINYINT\n"
            "        CHECK(`icmp_type` >= 0 AND `icmp_type` <= 255)\n"
            ")",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    HAPAssert(!errCode);
}

void HAPPlatformWiFiRouterCreate(HAPPlatformWiFiRouterRef wiFiRouter, const HAPPlatformWiFiRouterOptions* options) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(options);

    HAPError err;
    int errCode;

    HAPRawBufferZero(wiFiRouter, sizeof *wiFiRouter);

    // Set up synchronization.
    {
        int e;

        // Mutex for synchronization of the read-write lock.
        e = pthread_mutex_init(&wiFiRouter->synchronization.mutex, /* attr: */ NULL);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_init failed: %d.", e);
            HAPFatalError();
        }

        // Thread local storage key to keep track of recursive access requests.
        e = pthread_key_create(&wiFiRouter->synchronization.key, /* destructor: */ NULL);
        if (e) {
            HAPLogError(&logObject, "pthread_key_create failed: %d.", e);
            HAPFatalError();
        }
        e = pthread_setspecific(wiFiRouter->synchronization.key, 0);
        if (e) {
            HAPLogError(&logObject, "pthread_setspecific failed: %d.", e);
            HAPFatalError();
        }

        // Read-write lock for synchronization across threads.
        e = pthread_rwlock_init(&wiFiRouter->synchronization.rwLock, /* attr: */ NULL);
        if (e) {
            HAPLogError(&logObject, "pthread_rwlock_init failed: %d.", e);
            HAPFatalError();
        }
    }

    // Set up database.
    SetupSQLite();
    ConnectDatabase(&wiFiRouter->db, options->dbFile);

    // Initialize configuration.
    err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(wiFiRouter);
    HAPAssert(!err);
    err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
    HAPAssert(!err);

    CreateSchema(wiFiRouter->db);

    // Main WAN.
    {
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier = kHAPPlatformWiFiRouterWANIdentifier_Main;
        HAPPlatformWiFiRouterWANType wanType = kHAPPlatformWiFiRouterWANType_DHCP;

        size_t i;
        for (i = 0; i < HAPArrayCount(wiFiRouter->wans); i++) {
            if (wiFiRouter->wans[i].wanIdentifier == wanIdentifier) {
                break;
            }
        }
        if (i == HAPArrayCount(wiFiRouter->wans)) {
            for (i = 0; i < HAPArrayCount(wiFiRouter->wans); i++) {
                if (!wiFiRouter->wans[i].wanIdentifier) {
                    break;
                }
            }
        }
        HAPAssert(i < HAPArrayCount(wiFiRouter->wans));
        wiFiRouter->wans[i].wanIdentifier = wanIdentifier;
        wiFiRouter->wans[i].wanStatus = 0;

        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_wans` (\n"
                    "    `id`,\n"
                    "    `type`\n"
                    ") VALUES (\n"
                    "    :id,\n"
                    "    :type\n"
                    ") ON CONFLICT(`id`) DO NOTHING",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            HAPAssert(!errCode);

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), wanIdentifier);
                errCode = errCode || sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":type"), wanType);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
            HAPAssert(!err);
        }
    }

    // Main network client group.
    {
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = kHAPPlatformWiFiRouterGroupIdentifier_Main;
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_groups` (\n"
                    "    `id`\n"
                    ") VALUES (\n"
                    "    :id\n"
                    ") ON CONFLICT(`id`) DO NOTHING",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            HAPAssert(!errCode);

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), groupIdentifier);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
            HAPAssert(!err);
        }
    }

    // Commit configuration.
    err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
    HAPAssert(!err);
    HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(wiFiRouter);

    HAPPlatformWiFiRouterSetReady(wiFiRouter, true);

    // Satellites.
    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->satellites); i++) {
        wiFiRouter->satellites[i].satelliteStatus = kHAPPlatformWiFiRouterSatelliteStatus_Connected;
    }
}

void HAPPlatformWiFiRouterRelease(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wiFiRouter->db);

    sqlite3_close_safe(wiFiRouter->db);

    // Tear down synchronization.
    {
        int e;

        e = pthread_rwlock_destroy(&wiFiRouter->synchronization.rwLock);
        if (e) {
            HAPLogError(&logObject, "pthread_rwlock_destroy failed: %d.", e);
        }
        e = pthread_key_delete(wiFiRouter->synchronization.key);
        if (e) {
            HAPLogError(&logObject, "pthread_key_delete failed: %d.", e);
        }
        e = pthread_mutex_destroy(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_destroy failed: %d.", e);
        }
    }

    HAPRawBufferZero(wiFiRouter, sizeof *wiFiRouter);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPPlatformWiFiRouterSetDelegate(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterDelegate* _Nullable delegate) {
    HAPPrecondition(wiFiRouter);

    if (delegate) {
        wiFiRouter->delegate = *delegate;
    } else {
        HAPRawBufferZero(&wiFiRouter->delegate, sizeof wiFiRouter->delegate);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err = kHAPError_None;
    int e;

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    if (!numSharedAccessRefs && !numExclusiveAccessRefs) {
        e = pthread_mutex_lock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_lock failed: %d.", e);
            HAPFatalError();
        }
        {
            e = pthread_rwlock_tryrdlock(&wiFiRouter->synchronization.rwLock);
            if (e) {
                HAPLogError(&logObject, "pthread_rwlock_tryrdlock failed: %d.", e);
                err = kHAPError_Busy;
            }
        }
        e = pthread_mutex_unlock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_unlock failed: %d.", e);
            HAPFatalError();
        }
    }
    if (err) {
        return err;
    }

    numSharedAccessRefs++;
    if (!numSharedAccessRefs) {
        HAPLogError(&logObject, "%s: Reached recursion limit.", __func__);
        HAPFatalError();
    }
    HAPLogDebug(&logObject, "Shared access reference count: %u.", numSharedAccessRefs);

    tls = (void*) (uintptr_t)(numSharedAccessRefs << 0 | numExclusiveAccessRefs << 8);
    e = pthread_setspecific(wiFiRouter->synchronization.key, tls);
    if (e) {
        HAPLogError(&logObject, "pthread_setspecific failed: %d.", e);
        HAPFatalError();
    }

    wiFiRouter->synchronization.numAccessRefs++;
    HAPAssert(wiFiRouter->synchronization.numAccessRefs);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    return kHAPError_None;
}

void HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    int e;

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    if (!numSharedAccessRefs) {
        HAPLogError(&logObject, "%s: Unbalanced call while access was not acquired.", __func__);
        HAPFatalError();
    }
    numSharedAccessRefs--;
    HAPLogDebug(&logObject, "Shared access reference count: %u.", numSharedAccessRefs);

    tls = (void*) (uintptr_t)(numSharedAccessRefs << 0 | numExclusiveAccessRefs << 8);
    e = pthread_setspecific(wiFiRouter->synchronization.key, tls);
    if (e) {
        HAPLogError(&logObject, "pthread_setspecific failed: %d.", e);
        HAPFatalError();
    }

    if (!numSharedAccessRefs && !numExclusiveAccessRefs) {
        e = pthread_mutex_lock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_lock failed: %d.", e);
            HAPFatalError();
        }
        {
            e = pthread_rwlock_unlock(&wiFiRouter->synchronization.rwLock);
            if (e) {
                HAPLogError(&logObject, "pthread_rwlock_unlock failed: %d.", e);
                HAPFatalError();
            }
        }
        e = pthread_mutex_unlock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_unlock failed: %d.", e);
            HAPFatalError();
        }
    }

    HAPAssert(wiFiRouter->synchronization.numAccessRefs);
    wiFiRouter->synchronization.numAccessRefs--;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterHasSharedConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    return numSharedAccessRefs != 0 || numExclusiveAccessRefs != 0;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    HAPError err = kHAPError_None;
    int e;

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    if (!numExclusiveAccessRefs) {
        e = pthread_mutex_lock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_lock failed: %d.", e);
            HAPFatalError();
        }
        {
            if (numSharedAccessRefs) {
                e = pthread_rwlock_unlock(&wiFiRouter->synchronization.rwLock);
                if (e) {
                    HAPLogError(&logObject, "pthread_rwlock_unlock failed: %d.", e);
                    HAPFatalError();
                }
            }

            e = pthread_rwlock_trywrlock(&wiFiRouter->synchronization.rwLock);
            if (e) {
                HAPLogError(&logObject, "pthread_rwlock_trywrlock failed: %d.", e);
                err = kHAPError_Busy;

                if (numSharedAccessRefs) {
                    e = pthread_rwlock_tryrdlock(&wiFiRouter->synchronization.rwLock);
                    if (e) {
                        HAPLogError(&logObject, "pthread_rwlock_tryrdlock failed: %d.", e);
                        HAPFatalError();
                    }
                }
            }
        }
        e = pthread_mutex_unlock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_unlock failed: %d.", e);
            HAPFatalError();
        }
    }
    if (err) {
        return err;
    }

    numExclusiveAccessRefs++;
    if (!numExclusiveAccessRefs) {
        HAPLogError(&logObject, "%s: Reached recursion limit.", __func__);
        HAPFatalError();
    }
    HAPLogDebug(&logObject, "Exclusive access reference count: %u.", numExclusiveAccessRefs);

    tls = (void*) (uintptr_t)(numSharedAccessRefs << 0 | numExclusiveAccessRefs << 8);
    e = pthread_setspecific(wiFiRouter->synchronization.key, tls);
    if (e) {
        HAPLogError(&logObject, "pthread_setspecific failed: %d.", e);
        HAPFatalError();
    }

    wiFiRouter->synchronization.numAccessRefs++;
    HAPAssert(wiFiRouter->synchronization.numAccessRefs);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    return kHAPError_None;
}

void HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    int e;

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    if (!numExclusiveAccessRefs) {
        HAPLogError(&logObject, "%s: Unbalanced call while access was not acquired.", __func__);
        HAPFatalError();
    }
    numExclusiveAccessRefs--;
    HAPLogDebug(&logObject, "Exclusive access reference count: %u.", numExclusiveAccessRefs);

    tls = (void*) (uintptr_t)(numSharedAccessRefs << 0 | numExclusiveAccessRefs << 8);
    e = pthread_setspecific(wiFiRouter->synchronization.key, tls);
    if (e) {
        HAPLogError(&logObject, "pthread_setspecific failed: %d.", e);
        HAPFatalError();
    }

    if (!numExclusiveAccessRefs) {
        e = pthread_mutex_lock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_lock failed: %d.", e);
            HAPFatalError();
        }
        {
            e = pthread_rwlock_unlock(&wiFiRouter->synchronization.rwLock);
            if (e) {
                HAPLogError(&logObject, "pthread_rwlock_unlock failed: %d.", e);
                HAPFatalError();
            }

            if (numSharedAccessRefs) {
                e = pthread_rwlock_tryrdlock(&wiFiRouter->synchronization.rwLock);
                if (e) {
                    HAPLogError(&logObject, "pthread_rwlock_tryrdlock failed: %d.", e);
                    HAPFatalError();
                }
            }
        }
        e = pthread_mutex_unlock(&wiFiRouter->synchronization.mutex);
        if (e) {
            HAPLogError(&logObject, "pthread_mutex_unlock failed: %d.", e);
            HAPFatalError();
        }
    }

    HAPAssert(wiFiRouter->synchronization.numAccessRefs);
    wiFiRouter->synchronization.numAccessRefs--;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

HAP_RESULT_USE_CHECK
static bool HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    void* tls = pthread_getspecific(wiFiRouter->synchronization.key);
    uint8_t numSharedAccessRefs HAP_UNUSED = (uintptr_t) tls >> 0 & 0xFF;
    uint8_t numExclusiveAccessRefs = (uintptr_t) tls >> 8 & 0xFF;

    return numExclusiveAccessRefs != 0;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);

    return !wiFiRouter->synchronization.numAccessRefs && !wiFiRouter->edits.isEditing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static HAPError
        MarkClientDirty(HAPPlatformWiFiRouterRef wiFiRouter, HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);

    for (size_t i = 0; i < wiFiRouter->edits.numClients; i++) {
        if (wiFiRouter->edits.clients[i] == clientIdentifier) {
            return kHAPError_None;
        }
    }
    if (wiFiRouter->edits.numClients >= HAPArrayCount(wiFiRouter->edits.clients)) {
        HAPLog(&logObject, "Too many network client profile edits in one bulk operation.");
        return kHAPError_OutOfResources;
    }
    wiFiRouter->edits.clients[wiFiRouter->edits.numClients++] = clientIdentifier;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError MarkMACAddressDirty(HAPPlatformWiFiRouterRef wiFiRouter, const HAPMACAddress* macAddress) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(macAddress);

    for (size_t i = 0; i < wiFiRouter->edits.numMACAddresses; i++) {
        if (HAPRawBufferAreEqual(
                    wiFiRouter->edits.macAddresses[i].bytes, macAddress->bytes, sizeof macAddress->bytes)) {
            return kHAPError_None;
        }
    }
    if (wiFiRouter->edits.numMACAddresses >= HAPArrayCount(wiFiRouter->edits.macAddresses)) {
        HAPLog(&logObject, "Too many MAC address edits in one bulk operation.");
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(
            &wiFiRouter->edits.macAddresses[wiFiRouter->edits.numMACAddresses++], macAddress, sizeof *macAddress);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterBeginConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);

    int errCode;

    errCode = sqlite3_exec(
            wiFiRouter->db,
            "BEGIN IMMEDIATE TRANSACTION",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    wiFiRouter->edits.isEditing = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterCommitConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(!wiFiRouter->edits.isEditingWANFirewall);
    HAPPrecondition(!wiFiRouter->edits.isEditingLANFirewall);

    int errCode;

    errCode = sqlite3_exec(
            wiFiRouter->db,
            "COMMIT TRANSACTION",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    for (size_t i = 0; i < wiFiRouter->edits.numClients; i++) {
        for (size_t j = 0; j < HAPArrayCount(wiFiRouter->connections); j++) {
            if (!wiFiRouter->connections[j].isActive) {
                continue;
            }
            if (wiFiRouter->connections[j].clientIdentifier == wiFiRouter->edits.clients[i]) {
                HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[j].macAddress);
            }
        }
    }
    for (size_t i = 0; i < wiFiRouter->edits.numMACAddresses; i++) {
        for (size_t j = 0; j < HAPArrayCount(wiFiRouter->connections); j++) {
            if (!wiFiRouter->connections[j].isActive) {
                continue;
            }
            if (HAPRawBufferAreEqual(
                        wiFiRouter->connections[j].macAddress.bytes,
                        wiFiRouter->edits.macAddresses[i].bytes,
                        sizeof wiFiRouter->edits.macAddresses[i].bytes)) {
                HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[j].macAddress);
            }
        }
    }

    HAPRawBufferZero(&wiFiRouter->edits, sizeof wiFiRouter->edits);
    wiFiRouter->edits.isEditing = false;
    return kHAPError_None;
}

void HAPPlatformWiFiRouterRollbackConfigurationChange(HAPPlatformWiFiRouterRef wiFiRouter) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);

    (void) sqlite3_exec(
            wiFiRouter->db,
            "ROLLBACK TRANSACTION",
            /* callback: */ NULL,
            /* context: */ NULL,
            /* errmsg: */ NULL);

    HAPRawBufferZero(&wiFiRouter->edits, sizeof wiFiRouter->edits);
    wiFiRouter->edits.isEditing = false;
    wiFiRouter->edits.isEditingWANFirewall = false;
    wiFiRouter->edits.isEditingLANFirewall = false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsReady(HAPPlatformWiFiRouterRef wiFiRouter, bool* isReady) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(isReady);

    *isReady = wiFiRouter->isReady;
    return kHAPError_None;
}

void HAPPlatformWiFiRouterSetReady(HAPPlatformWiFiRouterRef wiFiRouter, bool ready) {
    HAPPrecondition(wiFiRouter);

    if (ready == wiFiRouter->isReady) {
        return;
    }

    HAPLogInfo(&logObject, "Setting router status to %s.", ready ? "ready" : "not ready");
    wiFiRouter->isReady = ready;
    if (wiFiRouter->delegate.handleReadyStateChanged) {
        wiFiRouter->delegate.handleReadyStateChanged(wiFiRouter, wiFiRouter->delegate.context);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterIsManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool* isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(isEnabled);

    HAPError err;
    int errCode;

    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = kHAPPlatformWiFiRouterGroupIdentifier_Restricted;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `id`\n"
            "FROM `hap_groups`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), groupIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                *isEnabled = false;
                err = kHAPError_None;
            } else if (errCode == SQLITE_ROW) {
                *isEnabled = true;
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError SetManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);

    HAPError err;
    int errCode;

    HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = kHAPPlatformWiFiRouterGroupIdentifier_Restricted;

    if (!isEnabled) {
        for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
            if (wiFiRouter->connections[i].isActive && wiFiRouter->connections[i].clientIdentifier) {
                HAPPlatformWiFiRouterDisconnectClient(wiFiRouter, &wiFiRouter->connections[i].macAddress);
            }
        }
    }

    if (isEnabled) {
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "SELECT `id` FROM `hap_wans`\n"
                    "WHERE `type` = :type\n"
                    "LIMIT 1",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int(
                        stmt, sqlite3_bind_parameter_index(stmt, ":type"), kHAPPlatformWiFiRouterWANType_BridgeMode);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    } else if (errCode == SQLITE_ROW) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            HAPLog(&logObject, "Managed Network cannot be enabled: A WAN is in bridge mode.");
                            err = kHAPError_InvalidState;
                        }
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
            if (err) {
                return err;
            }
        }
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_groups` (\n"
                    "    `id`\n"
                    ") VALUES (\n"
                    "    :id\n"
                    ") ON CONFLICT(`id`) DO NOTHING",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), groupIdentifier);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
            if (err) {
                return err;
            }
        }
    } else {
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "DELETE FROM `hap_clients`",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
            sqlite3_finalize_safe(stmt);
            if (err) {
                return err;
            }
        }
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "DELETE FROM `sqlite_sequence`\n"
                    "WHERE `name` = 'hap_clients'",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
            sqlite3_finalize_safe(stmt);
            if (err) {
                return err;
            }
        }
        {
            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "DELETE FROM `hap_groups`\n"
                    "WHERE `id` = :id",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), groupIdentifier);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
            if (err) {
                return err;
            }
        }
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterSetManagedNetworkEnabled(HAPPlatformWiFiRouterRef wiFiRouter, bool isEnabled) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);

    HAPError err;

    err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    err = SetManagedNetworkEnabled(wiFiRouter, isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        HAPPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
        return err;
    }

    err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
        return err;
    }

    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateWANs(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateWANsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(callback);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `id`\n"
            "FROM `hap_wans`",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        wiFiRouter->synchronization.enumerationNestingLevel++;
        HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
        {
            bool shouldContinue = true;
            while (shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                sqlite_int64 wanIdentifier = sqlite3_column_int64(stmt, /* iCol: */ 0);

                callback(context, wiFiRouter, (HAPPlatformWiFiRouterWANIdentifier) wanIdentifier, &shouldContinue);
            }
            if (errCode == SQLITE_ROW || errCode == SQLITE_DONE) {
                err = kHAPError_None;
            }
        }
        HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
        wiFiRouter->synchronization.enumerationNestingLevel--;
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        bool* exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(exists);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `id`\n"
            "FROM `hap_wans`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), wanIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                *exists = false;
                err = kHAPError_None;
            } else if (errCode == SQLITE_ROW) {
                *exists = true;
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType* wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanType);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `type`\n"
            "FROM `hap_wans`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), wanIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                int type = sqlite3_column_int(stmt, /* iCol: */ 0);

                *wanType = (HAPPlatformWiFiRouterWANType) type;
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError WANSetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wanIdentifier);

    HAPError err;
    int errCode;

    HAPLogInfo(&logObject, "Setting WAN %lu type to %u.", (unsigned long) wanIdentifier, wanType);
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "UPDATE `hap_wans` SET\n"
                "    `type` = :type\n"
                "WHERE `id` = :id",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), wanIdentifier);
            if (wanType) {
                errCode = errCode || sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":type"), wanType);
            }
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    if (!sqlite3_changes(wiFiRouter->db)) {
                        HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
                        err = kHAPError_InvalidState;
                    } else {
                        err = kHAPError_None;
                    }
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    bool managedNetworkStateChanged = false;
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "SELECT\n"
                "    `id`\n"
                "FROM `hap_wans`\n"
                "WHERE `type` = :type\n"
                "LIMIT 1",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int(
                    stmt, sqlite3_bind_parameter_index(stmt, ":type"), kHAPPlatformWiFiRouterWANType_BridgeMode);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                } else if (errCode == SQLITE_ROW) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        HAPLog(&logObject, "Disabling managed network because WAN type is changed to bridge mode.");
                        err = SetManagedNetworkEnabled(wiFiRouter, /* isEnabled: */ false);
                        if (err) {
                            HAPAssert(err == kHAPError_Unknown);
                        }
                        managedNetworkStateChanged = true;
                    }
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }

    if (managedNetworkStateChanged) {
        if (wiFiRouter->delegate.handleManagedNetworkStateChanged) {
            wiFiRouter->delegate.handleManagedNetworkStateChanged(wiFiRouter, wiFiRouter->delegate.context);
        }
    }
    if (wiFiRouter->delegate.handleWANConfigurationChanged) {
        wiFiRouter->delegate.handleWANConfigurationChanged(wiFiRouter, wiFiRouter->delegate.context);
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType wanType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(wanIdentifier);

    HAPError err;

    err = HAPPlatformWiFiRouterBeginConfigurationChange(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    err = WANSetType(wiFiRouter, wanIdentifier, wanType);
    if (err) {
        HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
        HAPPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
        return err;
    }

    err = HAPPlatformWiFiRouterCommitConfigurationChange(wiFiRouter);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPPlatformWiFiRouterRollbackConfigurationChange(wiFiRouter);
        return err;
    }

    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANGetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus* wanStatus) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanStatus);

    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->wans); i++) {
        if (wiFiRouter->wans[i].wanIdentifier == wanIdentifier) {
            *wanStatus = wiFiRouter->wans[i].wanStatus;
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus wanStatus) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(wanIdentifier);
    HAPPrecondition(wanStatus);

    HAPLogInfo(&logObject, "Setting WAN %lu status to %u.", (unsigned long) wanIdentifier, wanStatus);
    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->wans); i++) {
        if (wiFiRouter->wans[i].wanIdentifier == wanIdentifier) {
            if (wiFiRouter->wans[i].wanStatus != wanStatus) {
                wiFiRouter->wans[i].wanStatus = wanStatus;
                if (wiFiRouter->delegate.handleWANStatusChanged) {
                    wiFiRouter->delegate.handleWANStatusChanged(wiFiRouter, wiFiRouter->delegate.context);
                }
            }
            return kHAPError_None;
        }
    }
    HAPLog(&logObject, "WAN %lu not found.", (unsigned long) wanIdentifier);
    return kHAPError_InvalidState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterEnumerateClients(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterEnumerateClientsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(callback);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `id`\n"
            "FROM `hap_clients`",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        wiFiRouter->synchronization.enumerationNestingLevel++;
        HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
        {
            bool shouldContinue = true;
            while (shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                sqlite_int64 clientIdentifier = sqlite3_column_int64(stmt, /* iCol: */ 0);

                callback(
                        context, wiFiRouter, (HAPPlatformWiFiRouterClientIdentifier) clientIdentifier, &shouldContinue);
            }
            if (errCode == SQLITE_ROW || errCode == SQLITE_DONE) {
                err = kHAPError_None;
            }
        }
        HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
        wiFiRouter->synchronization.enumerationNestingLevel--;
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientExists(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        bool* exists) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(exists);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `id`\n"
            "FROM `hap_clients`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                *exists = false;
                err = kHAPError_None;
            } else if (errCode == SQLITE_ROW) {
                *exists = true;
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterAddClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential,
        HAPPlatformWiFiRouterClientIdentifier* clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(groupIdentifier);
    HAPPrecondition(credential);
    HAPPrecondition(clientIdentifier);

    HAPError err;
    int errCode;

    bool isEnabled;
    err = HAPPlatformWiFiRouterIsManagedNetworkEnabled(wiFiRouter, &isEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!isEnabled) {
        HAPLog(&logObject, "Network client management is only available when managed network is enabled.");
        return kHAPError_Unknown;
    }

    if (credential->type == kHAPPlatformWiFiRouterCredentialType_MACAddress) {
        err = MarkMACAddressDirty(wiFiRouter, &credential->_.macAddress);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "INSERT INTO `hap_clients` (\n"
                "    `credential_mac`,\n"
                "    `credential_passphrase`,\n"
                "    `credential_psk`\n"
                ") VALUES (\n"
                "    :credential_mac,\n"
                "    :credential_passphrase,\n"
                "    :credential_psk\n"
                ")",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            switch (credential->type) {
                case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                    errCode = sqlite3_bind_blob(
                            stmt,
                            sqlite3_bind_parameter_index(stmt, ":credential_mac"),
                            credential->_.macAddress.bytes,
                            sizeof credential->_.macAddress.bytes,
                            SQLITE_STATIC);
                    break;
                }
                case kHAPPlatformWiFiRouterCredentialType_PSK: {
                    switch (credential->_.psk.type) {
                        case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                            errCode = sqlite3_bind_text(
                                    stmt,
                                    sqlite3_bind_parameter_index(stmt, ":credential_passphrase"),
                                    credential->_.psk._.passphrase.stringValue,
                                    /* nByte: */ -1,
                                    SQLITE_STATIC);
                            break;
                        }
                        case kHAPWiFiWPAPersonalCredentialType_PSK: {
                            errCode = sqlite3_bind_blob(
                                    stmt,
                                    sqlite3_bind_parameter_index(stmt, ":credential_psk"),
                                    credential->_.psk._.psk.bytes,
                                    sizeof credential->_.psk._.psk.bytes,
                                    SQLITE_STATIC);
                            break;
                        }
                    }
                    break;
                }
            }
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    sqlite3_int64 rawClientIdentifier = sqlite3_last_insert_rowid(wiFiRouter->db);

                    *clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "INSERT INTO `hap_client_groups` (\n"
                "    `client`,\n"
                "    `group`\n"
                ") VALUES (\n"
                "    :client,\n"
                "    :group\n"
                ")",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), *clientIdentifier);
            errCode =
                    errCode || sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":group"), groupIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterRemoveClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(clientIdentifier);

    HAPError err;
    int errCode;

    err = MarkClientDirty(wiFiRouter, clientIdentifier);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "DELETE FROM `hap_clients`\n"
            "WHERE `id` = :id",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                err = kHAPError_None;
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier* groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(groupIdentifier);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `group`\n"
            "FROM `hap_client_groups`\n"
            "WHERE `client` = :client\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                sqlite_int64 rawGroupIdentifier = sqlite3_column_int64(stmt, /* iCol: */ 0);

                *groupIdentifier = (HAPPlatformWiFiRouterGroupIdentifier) rawGroupIdentifier;
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetGroupIdentifier(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterGroupIdentifier groupIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(groupIdentifier);

    HAPError err;
    int errCode;

    err = MarkClientDirty(wiFiRouter, clientIdentifier);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "UPDATE `hap_client_groups` SET\n"
            "    `group` = :group\n"
            "WHERE `client` = :client",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
        errCode = errCode || sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":group"), groupIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                if (!sqlite3_changes(wiFiRouter->db)) {
                    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                    err = kHAPError_InvalidState;
                } else {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetCredentialType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterCredentialType* credentialType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credentialType);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `credential_mac` IS NOT NULL AS `credential_mac`,\n"
            "    `credential_passphrase` IS NOT NULL AS `credential_passphrase`,\n"
            "    `credential_psk` IS NOT NULL AS `credential_psk`\n"
            "FROM `hap_clients`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                int hasCredentialMAC = sqlite3_column_int(stmt, /* iCol: */ 0);
                int hasCredentialPassphrase = sqlite3_column_int(stmt, /* iCol: */ 1);
                int hasCredentialPSK = sqlite3_column_int(stmt, /* iCol: */ 2);

                if (hasCredentialMAC) {
                    *credentialType = kHAPPlatformWiFiRouterCredentialType_MACAddress;
                } else {
                    HAPAssert(hasCredentialPassphrase || hasCredentialPSK);
                    *credentialType = kHAPPlatformWiFiRouterCredentialType_PSK;
                }
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetMACAddressCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPMACAddress* credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credential);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `credential_mac`\n"
            "FROM `hap_clients`\n"
            "WHERE\n"
            "    `id` = :id AND\n"
            "    `credential_mac` IS NOT NULL\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject,
                       "MAC address based network client profile %lu not found.",
                       (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                const void* _Nullable rawCredentialMAC = sqlite3_column_blob(stmt, /* iCol: */ 0);
                int numCredentialMACBytes = sqlite3_column_bytes(stmt, /* iCol: */ 0);

                if (!rawCredentialMAC || (size_t) numCredentialMACBytes != sizeof credential->bytes) {
                    HAPLog(&logObject,
                           "Failed to fetch credential for MAC address based network client profile %lu.",
                           (unsigned long) clientIdentifier);
                } else {
                    HAPRawBufferCopyBytes(
                            credential->bytes, HAPNonnullVoid(rawCredentialMAC), (size_t) numCredentialMACBytes);
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientSetCredential(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterClientCredential* credential) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(credential);

    HAPError err;
    int errCode;

    err = MarkClientDirty(wiFiRouter, clientIdentifier);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    if (credential->type == kHAPPlatformWiFiRouterCredentialType_MACAddress) {
        err = MarkMACAddressDirty(wiFiRouter, &credential->_.macAddress);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "UPDATE `hap_clients` SET\n"
            "    `credential_mac` = :credential_mac,\n"
            "    `credential_passphrase` = :credential_passphrase,\n"
            "    `credential_psk` = :credential_psk\n"
            "WHERE `id` = :id",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        switch (credential->type) {
            case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                errCode = errCode || sqlite3_bind_blob(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":credential_mac"),
                                             credential->_.macAddress.bytes,
                                             sizeof credential->_.macAddress.bytes,
                                             SQLITE_STATIC);
                break;
            }
            case kHAPPlatformWiFiRouterCredentialType_PSK: {
                switch (credential->_.psk.type) {
                    case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                        errCode = errCode || sqlite3_bind_text(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":credential_passphrase"),
                                                     credential->_.psk._.passphrase.stringValue,
                                                     /* nByte: */ -1,
                                                     SQLITE_STATIC);
                        break;
                    }
                    case kHAPWiFiWPAPersonalCredentialType_PSK: {
                        errCode = errCode || sqlite3_bind_blob(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":credential_psk"),
                                                     credential->_.psk._.psk.bytes,
                                                     sizeof credential->_.psk._.psk.bytes,
                                                     SQLITE_STATIC);
                        break;
                    }
                }
                break;
            }
        }
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                if (!sqlite3_changes(wiFiRouter->db)) {
                    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                    err = kHAPError_InvalidState;
                } else {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetWANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `wan_full_access`\n"
            "FROM `hap_clients`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                int wanFullAccess = sqlite3_column_int(stmt, /* iCol: */ 0);

                if (wanFullAccess) {
                    *firewallType = kHAPPlatformWiFiRouterFirewallType_FullAccess;
                } else {
                    *firewallType = kHAPPlatformWiFiRouterFirewallType_Allowlist;
                }
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateWANFirewallRulesCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(callback);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `type`,\n"               // 0. HAPPlatformWiFiRouterWANFirewallRuleType.
            "    `sort_order`,\n"         // 1.
            "    `transport_protocol`,\n" // 2.
            "    `host_dns_name`,\n"      // 3.
            "    `host_ip_start`,\n"      // 4.
            "    `host_ip_end`,\n"        // 5.
            "    `host_port_start`,\n"    // 6.
            "    `host_port_end`,\n"      // 7.
            "    `icmp_protocol`,\n"      // 8.
            "    `icmp_type`\n"           // 9.
            "FROM (\n"
            "    SELECT\n"
            "        1 AS `type`,\n"
            "        `sort_order`,\n"
            "        `transport_protocol`,\n"
            "        `host_dns_name`,\n"
            "        `host_ip_start`,\n"
            "        `host_ip_end`,\n"
            "        `host_port_start`,\n"
            "        `host_port_end`,\n"
            "        NULL AS `icmp_protocol`,\n"
            "        NULL AS `icmp_type`\n"
            "    FROM `hap_client_firewalls_wan_port`\n"
            "    WHERE `client` = :client\n"
            "    UNION ALL\n"
            "    SELECT\n"
            "        2 AS `type`,\n"
            "        `sort_order`,\n"
            "        NULL AS `transport_protocol`,\n"
            "        `host_dns_name`,\n"
            "        `host_ip_start`,\n"
            "        `host_ip_end`,\n"
            "        NULL AS `host_port_start`,\n"
            "        NULL AS `host_port_end`,\n"
            "        `icmp_protocol`,\n"
            "        `icmp_type`\n"
            "    FROM `hap_client_firewalls_wan_icmp`\n"
            "    WHERE `client` = :client\n"
            ")\n"
            "ORDER BY `sort_order`",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
        if (!errCode) {
            wiFiRouter->synchronization.enumerationNestingLevel++;
            HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
            {
                bool shouldContinue = true;
                bool isValid = true;
                bool didPrefetch = false;
                while (isValid && shouldContinue && (didPrefetch || (errCode = sqlite3_step(stmt)) == SQLITE_ROW)) {
                    didPrefetch = false;

                    int type = sqlite3_column_int(stmt, /* iCol: */ 0);
                    sqlite_int64 sortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                    int transportProtocol = sqlite3_column_int(stmt, /* iCol: */ 2);

                    bool hasHostDNSName = sqlite3_column_type(stmt, /* iCol: */ 3) != SQLITE_NULL;
                    const unsigned char* _Nullable hostDNSNameValue = sqlite3_column_text(stmt, /* iCol: */ 3);
                    int numHostDNSNameBytes = sqlite3_column_bytes(stmt, /* iCol: */ 3);
                    unsigned char* _Nullable hostDNSName = NULL;
                    if (hasHostDNSName) {
                        hostDNSName = sqlite3_malloc(numHostDNSNameBytes + 1);
                        if (hostDNSName) {
                            HAPRawBufferCopyBytes(
                                    HAPNonnull(hostDNSName),
                                    HAPNonnull(hostDNSNameValue),
                                    (size_t)(numHostDNSNameBytes + 1));
                        }
                    }

                    bool hasHostIPStart = sqlite3_column_type(stmt, /* iCol: */ 4) != SQLITE_NULL;
                    const void* _Nullable hostIPStartValue = sqlite3_column_blob(stmt, /* iCol: */ 4);
                    int numHostIPStartBytes = sqlite3_column_bytes(stmt, /* iCol: */ 4);
                    void* _Nullable hostIPStart = NULL;
                    if (hasHostIPStart) {
                        hostIPStart = sqlite3_malloc(numHostIPStartBytes);
                        if (hostIPStart) {
                            HAPRawBufferCopyBytes(
                                    HAPNonnullVoid(hostIPStart),
                                    HAPNonnullVoid(hostIPStartValue),
                                    (size_t) numHostIPStartBytes);
                        }
                    }

                    bool hasHostIPEnd = sqlite3_column_type(stmt, /* iCol: */ 5) != SQLITE_NULL;
                    const void* _Nullable hostIPEndValue = sqlite3_column_blob(stmt, /* iCol: */ 5);
                    int numHostIPEndBytes = sqlite3_column_bytes(stmt, /* iCol: */ 5);
                    void* _Nullable hostIPEnd = NULL;
                    if (hasHostIPEnd) {
                        hostIPEnd = sqlite3_malloc(numHostIPEndBytes);
                        if (hostIPEnd) {
                            HAPRawBufferCopyBytes(
                                    HAPNonnullVoid(hostIPEnd),
                                    HAPNonnullVoid(hostIPEndValue),
                                    (size_t) numHostIPEndBytes);
                        }
                    }

                    int hostPortStart = sqlite3_column_int(stmt, /* iCol: */ 6);
                    int hostPortEnd = sqlite3_column_int(stmt, /* iCol: */ 7);
                    int icmpProtocol = sqlite3_column_int(stmt, /* iCol: */ 8);

                    bool hasICMPType = sqlite3_column_type(stmt, /* iCol: */ 9) != SQLITE_NULL;
                    int icmpType = sqlite3_column_int(stmt, /* iCol: */ 9);

                    switch (type) {
                        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
                            if (hasHostDNSName && !hostDNSName) {
                                HAPLog(&logObject,
                                       "Failed to fetch Port WAN Rule Host DNS Name for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else if (
                                    (hasHostIPStart && !hostIPStart) ||
                                    (hostIPStart && numHostIPStartBytes != 4 && numHostIPStartBytes != 16)) {
                                HAPLog(&logObject,
                                       "Failed to fetch Port WAN Rule Host IP Start for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else if (
                                    (hasHostIPEnd && !hostIPEnd) ||
                                    (hostIPEnd && numHostIPStartBytes != numHostIPEndBytes)) {
                                HAPLog(&logObject,
                                       "Failed to fetch Port WAN Rule Host IP End for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else {
                                HAPPlatformWiFiRouterPortWANFirewallRule firewallRule;
                                HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                                firewallRule.type = kHAPPlatformWiFiRouterWANFirewallRuleType_Port;
                                firewallRule.transportProtocol =
                                        (HAPPlatformWiFiRouterTransportProtocol) transportProtocol;
                                if (hostDNSName) {
                                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                                    firewallRule.host._.dnsNamePattern = (const char*) HAPNonnull(hostDNSName);
                                } else if (hostIPStart) {
                                    if (numHostIPStartBytes == 4) {
                                        if (hostIPEnd) {
                                            firewallRule.host.type =
                                                    kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                                            firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                    HAPNonnullVoid(hostIPEnd),
                                                    (size_t) numHostIPEndBytes);
                                        } else {
                                            firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                                            firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddress._.ipv4.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                        }
                                    } else {
                                        HAPAssert(numHostIPStartBytes == 16);
                                        if (hostIPEnd) {
                                            firewallRule.host.type =
                                                    kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                                            firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv6;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                    HAPNonnullVoid(hostIPEnd),
                                                    (size_t) numHostIPEndBytes);
                                        } else {
                                            firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                                            firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddress._.ipv6.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                        }
                                    }
                                } else {
                                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                                }
                                firewallRule.hostPortRange.startPort = (HAPNetworkPort) hostPortStart;
                                if (hostPortEnd) {
                                    firewallRule.hostPortRange.endPort = (HAPNetworkPort) hostPortEnd;
                                } else {
                                    firewallRule.hostPortRange.endPort = firewallRule.hostPortRange.startPort;
                                }
                                callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            }
                            break;
                        }
                        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
                            if (hasHostDNSName && !hostDNSName) {
                                HAPLog(&logObject,
                                       "Failed to fetch ICMP WAN Rule Host DNS Name for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else if (
                                    (hasHostIPStart && !hostIPStart) ||
                                    (hostIPStart && numHostIPStartBytes != 4 && numHostIPStartBytes != 16)) {
                                HAPLog(&logObject,
                                       "Failed to fetch ICMP WAN Rule Host IP Start for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else if (
                                    (hasHostIPEnd && !hostIPEnd) ||
                                    (hostIPEnd && numHostIPStartBytes != numHostIPEndBytes)) {
                                HAPLog(&logObject,
                                       "Failed to fetch ICMP WAN Rule Host IP End for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else {
                                HAPPlatformWiFiRouterICMPWANFirewallRule firewallRule;
                                HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                                firewallRule.type = kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP;
                                if (hostDNSName) {
                                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern;
                                    firewallRule.host._.dnsNamePattern = (const char*) HAPNonnull(hostDNSName);
                                } else if (hostIPStart) {
                                    if (numHostIPStartBytes == 4) {
                                        if (hostIPEnd) {
                                            firewallRule.host.type =
                                                    kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                                            firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv4;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                    HAPNonnullVoid(hostIPEnd),
                                                    (size_t) numHostIPEndBytes);
                                        } else {
                                            firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                                            firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv4;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddress._.ipv4.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                        }
                                    } else {
                                        HAPAssert(numHostIPStartBytes == 16);
                                        if (hostIPEnd) {
                                            firewallRule.host.type =
                                                    kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange;
                                            firewallRule.host._.ipAddressRange.version = kHAPIPAddressVersion_IPv6;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                    HAPNonnullVoid(hostIPEnd),
                                                    (size_t) numHostIPEndBytes);
                                        } else {
                                            firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_IPAddress;
                                            firewallRule.host._.ipAddress.version = kHAPIPAddressVersion_IPv6;
                                            HAPRawBufferCopyBytes(
                                                    firewallRule.host._.ipAddress._.ipv6.bytes,
                                                    HAPNonnullVoid(hostIPStart),
                                                    (size_t) numHostIPStartBytes);
                                        }
                                    }
                                } else {
                                    firewallRule.host.type = kHAPPlatformWiFiRouterWANHostURIType_Any;
                                }
                                firewallRule.icmpTypes[0].icmpProtocol =
                                        (HAPPlatformWiFiRouterICMPProtocol) icmpProtocol;
                                if (hasICMPType) {
                                    firewallRule.icmpTypes[0].typeValue = (uint8_t) icmpType;
                                    firewallRule.icmpTypes[0].typeValueIsSet = true;
                                }
                                firewallRule.numICMPTypes = 1;
                                while (isValid && shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                                    sqlite_int64 newSortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                                    if (newSortOrder != sortOrder) {
                                        didPrefetch = true;
                                        break;
                                    }

                                    icmpProtocol = sqlite3_column_int(stmt, /* iCol: */ 8);

                                    hasICMPType = sqlite3_column_type(stmt, /* iCol: */ 9) != SQLITE_NULL;
                                    icmpType = sqlite3_column_int(stmt, /* iCol: */ 9);

                                    bool isDuplicateICMPType = false;
                                    for (size_t i = 0; i < firewallRule.numICMPTypes; i++) {
                                        const HAPPlatformWiFiRouterICMPType* otherICMPType = &firewallRule.icmpTypes[i];

                                        if (icmpProtocol == otherICMPType->icmpProtocol &&
                                            (hasICMPType ? 1 : 0) == (otherICMPType->typeValueIsSet ? 1 : 0) &&
                                            (!hasICMPType || icmpType == otherICMPType->typeValue)) {
                                            isDuplicateICMPType = true;
                                            break;
                                        }
                                    }

                                    if (!isDuplicateICMPType) {
                                        if (firewallRule.numICMPTypes == HAPArrayCount(firewallRule.icmpTypes)) {
                                            HAPLog(&logObject, "WAN Rule with more ICMP type entries than supported.");
                                            isValid = false;
                                        } else {
                                            size_t i = firewallRule.numICMPTypes;
                                            firewallRule.icmpTypes[i].icmpProtocol =
                                                    (HAPPlatformWiFiRouterICMPProtocol) icmpProtocol;
                                            if (hasICMPType) {
                                                firewallRule.icmpTypes[i].typeValue = (uint8_t) icmpType;
                                                firewallRule.icmpTypes[i].typeValueIsSet = true;
                                            }
                                            firewallRule.numICMPTypes++;
                                        }
                                    }
                                }
                                callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            }
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    if (hostDNSName) {
                        sqlite3_free_safe(hostDNSName);
                    }
                    if (hostIPStart) {
                        sqlite3_free_safe(hostIPStart);
                    }
                    if (hostIPEnd) {
                        sqlite3_free_safe(hostIPEnd);
                    }
                    if (errCode == SQLITE_DONE) {
                        break;
                    }
                }
                if (isValid && (errCode == SQLITE_ROW || errCode == SQLITE_DONE)) {
                    err = kHAPError_None;
                }
            }
            HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
            wiFiRouter->synchronization.enumerationNestingLevel--;
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetWANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(!wiFiRouter->edits.isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;
    int errCode;

    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_wan_port`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_wan_icmp`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "UPDATE `hap_clients` SET\n"
                "    `wan_full_access` = :wan_full_access\n"
                "WHERE `id` = :id",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
            switch (firewallType) {
                case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                    errCode = errCode ||
                              sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":wan_full_access"), 1);
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                    errCode = errCode ||
                              sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":wan_full_access"), 0);
                    break;
                }
            }
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    if (!sqlite3_changes(wiFiRouter->db)) {
                        HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                        err = kHAPError_InvalidState;
                    } else {
                        if (firewallType == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
                            wiFiRouter->edits.firewallIndex = 0;
                            wiFiRouter->edits.isEditingWANFirewall = true;
                        }
                        err = kHAPError_None;
                    }
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddWANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterWANFirewallRule* firewallRule_) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(wiFiRouter->edits.isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallRule_);

    HAPError err;
    int errCode;

    switch (*(const HAPPlatformWiFiRouterWANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterWANFirewallRuleType_Port: {
            const HAPPlatformWiFiRouterPortWANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_wan_port` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `transport_protocol`,\n"
                    "    `host_dns_name`,\n"
                    "    `host_ip_start`,\n"
                    "    `host_ip_end`,\n"
                    "    `host_port_start`,\n"
                    "    `host_port_end`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :transport_protocol,\n"
                    "    :host_dns_name,\n"
                    "    :host_ip_start,\n"
                    "    :host_ip_end,\n"
                    "    :host_port_start,\n"
                    "    :host_port_end\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_Unknown;
            {
                errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                             wiFiRouter->edits.firewallIndex);
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":transport_protocol"),
                                             firewallRule->transportProtocol);
                switch (firewallRule->host.type) {
                    case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                        errCode = errCode || sqlite3_bind_text(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":host_dns_name"),
                                                     firewallRule->host._.dnsNamePattern,
                                                     /* nByte: */ -1,
                                                     SQLITE_STATIC);
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                        switch (firewallRule->host._.ipAddress.version) {
                            case kHAPIPAddressVersion_IPv4: {
                                errCode = errCode || sqlite3_bind_blob(
                                                             stmt,
                                                             sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                             firewallRule->host._.ipAddress._.ipv4.bytes,
                                                             sizeof firewallRule->host._.ipAddress._.ipv4.bytes,
                                                             SQLITE_STATIC);
                                break;
                            }
                            case kHAPIPAddressVersion_IPv6: {
                                errCode = errCode || sqlite3_bind_blob(
                                                             stmt,
                                                             sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                             firewallRule->host._.ipAddress._.ipv6.bytes,
                                                             sizeof firewallRule->host._.ipAddress._.ipv6.bytes,
                                                             SQLITE_STATIC);
                                break;
                            }
                        }
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                        switch (firewallRule->host._.ipAddressRange.version) {
                            case kHAPIPAddressVersion_IPv4: {
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                  firewallRule->host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                  SQLITE_STATIC);
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_end"),
                                                  firewallRule->host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                  SQLITE_STATIC);
                                break;
                            }
                            case kHAPIPAddressVersion_IPv6: {
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                  firewallRule->host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                  SQLITE_STATIC);
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_end"),
                                                  firewallRule->host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                  SQLITE_STATIC);
                                break;
                            }
                        }
                        break;
                    }
                }
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":host_port_start"),
                                             firewallRule->hostPortRange.startPort);
                if (firewallRule->hostPortRange.endPort != firewallRule->hostPortRange.startPort) {
                    errCode = errCode || sqlite3_bind_int(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":host_port_end"),
                                                 firewallRule->hostPortRange.endPort);
                }
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        wiFiRouter->edits.firewallIndex++;
                        HAPAssert(wiFiRouter->edits.firewallIndex);
                        err = kHAPError_None;
                    }
                }
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        case kHAPPlatformWiFiRouterWANFirewallRuleType_ICMP: {
            const HAPPlatformWiFiRouterICMPWANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_wan_icmp` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `host_dns_name`,\n"
                    "    `host_ip_start`,\n"
                    "    `host_ip_end`,\n"
                    "    `icmp_protocol`,\n"
                    "    `icmp_type`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :host_dns_name,\n"
                    "    :host_ip_start,\n"
                    "    :host_ip_end,\n"
                    "    :icmp_protocol,\n"
                    "    :icmp_type\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_None;
            for (size_t i = 0; !err && i < firewallRule->numICMPTypes; i++) {
                err = kHAPError_Unknown;
                errCode = sqlite3_reset(stmt) || sqlite3_clear_bindings(stmt);
                errCode = errCode ||
                          sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                             wiFiRouter->edits.firewallIndex);
                switch (firewallRule->host.type) {
                    case kHAPPlatformWiFiRouterWANHostURIType_Any: {
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_DNSNamePattern: {
                        errCode = errCode || sqlite3_bind_text(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":host_dns_name"),
                                                     firewallRule->host._.dnsNamePattern,
                                                     /* nByte: */ -1,
                                                     SQLITE_STATIC);
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_IPAddress: {
                        switch (firewallRule->host._.ipAddress.version) {
                            case kHAPIPAddressVersion_IPv4: {
                                errCode = errCode || sqlite3_bind_blob(
                                                             stmt,
                                                             sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                             firewallRule->host._.ipAddress._.ipv4.bytes,
                                                             sizeof firewallRule->host._.ipAddress._.ipv4.bytes,
                                                             SQLITE_STATIC);
                                break;
                            }
                            case kHAPIPAddressVersion_IPv6: {
                                errCode = errCode || sqlite3_bind_blob(
                                                             stmt,
                                                             sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                             firewallRule->host._.ipAddress._.ipv6.bytes,
                                                             sizeof firewallRule->host._.ipAddress._.ipv6.bytes,
                                                             SQLITE_STATIC);
                                break;
                            }
                        }
                        break;
                    }
                    case kHAPPlatformWiFiRouterWANHostURIType_IPAddressRange: {
                        switch (firewallRule->host._.ipAddressRange.version) {
                            case kHAPIPAddressVersion_IPv4: {
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                  firewallRule->host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv4.startAddress.bytes,
                                                  SQLITE_STATIC);
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_end"),
                                                  firewallRule->host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv4.endAddress.bytes,
                                                  SQLITE_STATIC);
                                break;
                            }
                            case kHAPIPAddressVersion_IPv6: {
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_start"),
                                                  firewallRule->host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv6.startAddress.bytes,
                                                  SQLITE_STATIC);
                                errCode = errCode ||
                                          sqlite3_bind_blob(
                                                  stmt,
                                                  sqlite3_bind_parameter_index(stmt, ":host_ip_end"),
                                                  firewallRule->host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                  sizeof firewallRule->host._.ipAddressRange._.ipv6.endAddress.bytes,
                                                  SQLITE_STATIC);
                                break;
                            }
                        }
                        break;
                    }
                }
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":icmp_protocol"),
                                             firewallRule->icmpTypes[i].icmpProtocol);
                if (firewallRule->icmpTypes[i].typeValueIsSet) {
                    errCode = errCode || sqlite3_bind_int(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":icmp_type"),
                                                 firewallRule->icmpTypes[i].typeValue);
                }
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            if (!err) {
                wiFiRouter->edits.firewallIndex++;
                HAPAssert(wiFiRouter->edits.firewallIndex);
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeWANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(wiFiRouter->edits.isEditingWANFirewall);
    HAPPrecondition(clientIdentifier);

    wiFiRouter->edits.firewallIndex = 0;
    wiFiRouter->edits.isEditingWANFirewall = false;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetLANFirewallType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType* firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `lan_full_access`\n"
            "FROM `hap_clients`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                int lanFullAccess = sqlite3_column_int(stmt, /* iCol: */ 0);

                if (lanFullAccess) {
                    *firewallType = kHAPPlatformWiFiRouterFirewallType_FullAccess;
                } else {
                    *firewallType = kHAPPlatformWiFiRouterFirewallType_Allowlist;
                }
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientEnumerateLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterClientEnumerateLANFirewallRulesCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(callback);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `type`,\n"                   // 0. HAPPlatformWiFiRouterLANFirewallRuleType.
            "    `sort_order`,\n"             // 1.
            "    `direction`,\n"              // 2.
            "    `transport_protocol`,\n"     // 3.
            "    `peer_group`,\n"             // 4.
            "    `ip`,\n"                     // 5.
            "    `port_start`,\n"             // 6.
            "    `port_end`,\n"               // 7.
            "    `advertisement_protocol`,\n" // 8.
            "    `service_type`,\n"           // 9.
            "    `advertisement_only`,\n"     // 10.
            "    `icmp_protocol`,\n"          // 11.
            "    `icmp_type`\n"               // 12.
            "FROM (\n"
            "    SELECT\n"
            "        1 AS `type`,\n"
            "        `sort_order`,\n"
            "        `direction`,\n"
            "        NULL AS `transport_protocol`,\n"
            "        `peer_group`,\n"
            "        `destination_ip` AS `ip`,\n"
            "        `destination_port` AS `port_start`,\n"
            "        NULL AS `port_end`,\n"
            "        NULL AS `advertisement_protocol`,\n"
            "        NULL AS `service_type`,\n"
            "        NULL AS `advertisement_only`,\n"
            "        NULL AS `icmp_protocol`,\n"
            "        NULL AS `icmp_type`\n"
            "    FROM `hap_client_firewalls_lan_multicast_bridging`\n"
            "    WHERE `client` = :client\n"
            "    UNION ALL\n"
            "    SELECT\n"
            "        2 AS `type`,\n"
            "        `sort_order`,\n"
            "        `direction`,\n"
            "        `transport_protocol`,\n"
            "        `peer_group`,\n"
            "        NULL AS `ip`,\n"
            "        `destination_port_start` AS `port_start`,\n"
            "        `destination_port_end` AS `port_end`,\n"
            "        NULL AS `advertisement_protocol`,\n"
            "        NULL AS `service_type`,\n"
            "        NULL AS `advertisement_only`,\n"
            "        NULL AS `icmp_protocol`,\n"
            "        NULL AS `icmp_type`\n"
            "    FROM `hap_client_firewalls_lan_static_port`\n"
            "    WHERE `client` = :client\n"
            "    UNION ALL\n"
            "    SELECT\n"
            "        3 AS `type`,\n"
            "        `sort_order`,\n"
            "        `direction`,\n"
            "        `transport_protocol`,\n"
            "        `peer_group`,\n"
            "        NULL AS `ip`,\n"
            "        NULL AS `port_start`,\n"
            "        NULL AS `port_end`,\n"
            "        `advertisement_protocol`,\n"
            "        `service_type`,\n"
            "        `advertisement_only`,\n"
            "        NULL AS `icmp_protocol`,\n"
            "        NULL AS `icmp_type`\n"
            "    FROM `hap_client_firewalls_lan_dynamic_port`\n"
            "    WHERE `client` = :client\n"
            "    UNION ALL\n"
            "    SELECT\n"
            "        4 AS `type`,\n"
            "        `sort_order`,\n"
            "        `direction`,\n"
            "        NULL AS `transport_protocol`,\n"
            "        `peer_group`,\n"
            "        NULL AS `ip`,\n"
            "        NULL AS `port_start`,\n"
            "        NULL AS `port_end`,\n"
            "        NULL AS `advertisement_protocol`,\n"
            "        NULL AS `service_type`,\n"
            "        NULL AS `advertisement_only`,\n"
            "        `icmp_protocol`,\n"
            "        `icmp_type`\n"
            "    FROM `hap_client_firewalls_lan_static_icmp`\n"
            "    WHERE `client` = :client\n"
            ")\n"
            "ORDER BY `sort_order`",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
        if (!errCode) {
            wiFiRouter->synchronization.enumerationNestingLevel++;
            HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
            {
                bool shouldContinue = true;
                bool isValid = true;
                bool didPrefetch = false;
                while (isValid && shouldContinue && (didPrefetch || (errCode = sqlite3_step(stmt)) == SQLITE_ROW)) {
                    didPrefetch = false;

                    int type = sqlite3_column_int(stmt, /* iCol: */ 0);
                    sqlite_int64 sortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                    int direction = sqlite3_column_int(stmt, /* iCol: */ 2);
                    int transportProtocol = sqlite3_column_int(stmt, /* iCol: */ 3);
                    sqlite_int64 peerGroup = sqlite3_column_int64(stmt, /* iCol: */ 4);

                    bool hasIP = sqlite3_column_type(stmt, /* iCol: */ 5) != SQLITE_NULL;
                    const void* _Nullable ipValue = sqlite3_column_blob(stmt, /* iCol: */ 5);
                    int numIPBytes = sqlite3_column_bytes(stmt, /* iCol: */ 5);
                    void* _Nullable ip = NULL;
                    if (hasIP) {
                        ip = sqlite3_malloc(numIPBytes);
                        if (ip) {
                            HAPRawBufferCopyBytes(HAPNonnullVoid(ip), HAPNonnullVoid(ipValue), (size_t) numIPBytes);
                        }
                    }

                    int portStart = sqlite3_column_int(stmt, /* iCol: */ 6);
                    int portEnd = sqlite3_column_int(stmt, /* iCol: */ 7);

                    int advertisementProtocol = sqlite3_column_int(stmt, /* iCol: */ 8);

                    bool hasServiceType = sqlite3_column_type(stmt, /* iCol: */ 9) != SQLITE_NULL;
                    const unsigned char* _Nullable serviceTypeValue = sqlite3_column_text(stmt, /* iCol: */ 9);
                    int numServiceTypeBytes = sqlite3_column_bytes(stmt, /* iCol: */ 9);
                    unsigned char* _Nullable serviceType = NULL;
                    if (hasServiceType) {
                        serviceType = sqlite3_malloc(numServiceTypeBytes + 1);
                        if (serviceType) {
                            HAPRawBufferCopyBytes(
                                    HAPNonnull(serviceType),
                                    HAPNonnull(serviceTypeValue),
                                    (size_t)(numServiceTypeBytes + 1));
                        }
                    }

                    int advertisementOnly = sqlite3_column_int(stmt, /* iCol: */ 10);

                    int icmpProtocol = sqlite3_column_int(stmt, /* iCol: */ 11);
                    bool hasICMPType = sqlite3_column_type(stmt, /* iCol: */ 12) != SQLITE_NULL;
                    int icmpType = sqlite3_column_int(stmt, /* iCol: */ 12);

                    switch (type) {
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
                            if (!ip || (numIPBytes != 4 && numIPBytes != 16)) {
                                HAPLog(&logObject,
                                       "Failed to fetch Multicast Bridging LAN Rule IP for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else {
                                HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule firewallRule;
                                HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                                firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging;
                                firewallRule.direction = (HAPPlatformWiFiRouterFirewallRuleDirection) direction;
                                firewallRule.peerGroupIdentifiers[0] = (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                firewallRule.numPeerGroupIdentifiers = 1;
                                if (numIPBytes == 4) {
                                    firewallRule.destinationIP.version = kHAPIPAddressVersion_IPv4;
                                    HAPRawBufferCopyBytes(
                                            firewallRule.destinationIP._.ipv4.bytes,
                                            HAPNonnullVoid(ip),
                                            (size_t) numIPBytes);
                                } else {
                                    HAPAssert(numIPBytes == 16);
                                    firewallRule.destinationIP.version = kHAPIPAddressVersion_IPv6;
                                    HAPRawBufferCopyBytes(
                                            firewallRule.destinationIP._.ipv6.bytes,
                                            HAPNonnullVoid(ip),
                                            (size_t) numIPBytes);
                                }
                                firewallRule.destinationPort = (HAPNetworkPort) portStart;
                                while (isValid && shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                                    sqlite_int64 newSortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                                    if (newSortOrder != sortOrder) {
                                        didPrefetch = true;
                                        break;
                                    }

                                    peerGroup = sqlite3_column_int64(stmt, /* iCol: */ 4);

                                    bool isDuplicatePeerGroup = false;
                                    for (size_t i = 0; i < firewallRule.numPeerGroupIdentifiers; i++) {
                                        if (peerGroup == firewallRule.peerGroupIdentifiers[i]) {
                                            isDuplicatePeerGroup = true;
                                            break;
                                        }
                                    }

                                    if (!isDuplicatePeerGroup) {
                                        if (firewallRule.numPeerGroupIdentifiers ==
                                            HAPArrayCount(firewallRule.peerGroupIdentifiers)) {
                                            HAPLog(&logObject,
                                                   "LAN Rule with more peerGroupIdentifiers than supported.");
                                            isValid = false;
                                        } else {
                                            size_t i = firewallRule.numPeerGroupIdentifiers;
                                            firewallRule.peerGroupIdentifiers[i] =
                                                    (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                            firewallRule.numPeerGroupIdentifiers++;
                                        }
                                    }
                                }
                                callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            }
                            break;
                        }
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
                            HAPPlatformWiFiRouterStaticPortLANFirewallRule firewallRule;
                            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort;
                            firewallRule.direction = (HAPPlatformWiFiRouterFirewallRuleDirection) direction;
                            firewallRule.transportProtocol = (HAPPlatformWiFiRouterTransportProtocol) transportProtocol;
                            firewallRule.peerGroupIdentifiers[0] = (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                            firewallRule.numPeerGroupIdentifiers = 1;
                            firewallRule.destinationPortRange.startPort = (HAPNetworkPort) portStart;
                            if (portEnd) {
                                firewallRule.destinationPortRange.endPort = (HAPNetworkPort) portEnd;
                            } else {
                                firewallRule.destinationPortRange.endPort = firewallRule.destinationPortRange.startPort;
                            }
                            while (isValid && shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                                sqlite_int64 newSortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                                if (newSortOrder != sortOrder) {
                                    didPrefetch = true;
                                    break;
                                }

                                peerGroup = sqlite3_column_int64(stmt, /* iCol: */ 4);

                                bool isDuplicatePeerGroup = false;
                                for (size_t i = 0; i < firewallRule.numPeerGroupIdentifiers; i++) {
                                    if (peerGroup == firewallRule.peerGroupIdentifiers[i]) {
                                        isDuplicatePeerGroup = true;
                                        break;
                                    }
                                }

                                if (!isDuplicatePeerGroup) {
                                    if (firewallRule.numPeerGroupIdentifiers ==
                                        HAPArrayCount(firewallRule.peerGroupIdentifiers)) {
                                        HAPLog(&logObject, "LAN Rule with more peerGroupIdentifiers than supported.");
                                        isValid = false;
                                    } else {
                                        size_t i = firewallRule.numPeerGroupIdentifiers;
                                        firewallRule.peerGroupIdentifiers[i] =
                                                (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                        firewallRule.numPeerGroupIdentifiers++;
                                    }
                                }
                            }
                            callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            break;
                        }
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
                            if (hasServiceType && !serviceType) {
                                HAPLog(&logObject,
                                       "Failed to fetch Dynamic Port LAN Rule Service for network client profile %lu.",
                                       (unsigned long) clientIdentifier);
                                isValid = false;
                            } else {
                                HAPPlatformWiFiRouterDynamicPortLANFirewallRule firewallRule;
                                HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                                firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort;
                                firewallRule.direction = (HAPPlatformWiFiRouterFirewallRuleDirection) direction;
                                firewallRule.transportProtocol =
                                        (HAPPlatformWiFiRouterTransportProtocol) transportProtocol;
                                firewallRule.peerGroupIdentifiers[0] = (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                firewallRule.numPeerGroupIdentifiers = 1;
                                firewallRule.serviceType.advertisementProtocol =
                                        (HAPPlatformWiFiRouterDynamicPortAdvertisementProtocol) advertisementProtocol;
                                if (hasServiceType) {
                                    switch (firewallRule.serviceType.advertisementProtocol) {
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                                            firewallRule.serviceType._.dns_sd.serviceType =
                                                    (const char*) HAPNonnullVoid(serviceType);
                                            break;
                                        }
                                        case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                                            firewallRule.serviceType._.ssdp.serviceTypeURI =
                                                    (const char*) HAPNonnullVoid(serviceType);
                                            break;
                                        }
                                    }
                                }
                                firewallRule.advertisementOnly = advertisementOnly != 0;
                                while (isValid && shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                                    sqlite_int64 newSortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                                    if (newSortOrder != sortOrder) {
                                        didPrefetch = true;
                                        break;
                                    }

                                    peerGroup = sqlite3_column_int64(stmt, /* iCol: */ 4);

                                    bool isDuplicatePeerGroup = false;
                                    for (size_t i = 0; i < firewallRule.numPeerGroupIdentifiers; i++) {
                                        if (peerGroup == firewallRule.peerGroupIdentifiers[i]) {
                                            isDuplicatePeerGroup = true;
                                            break;
                                        }
                                    }

                                    if (!isDuplicatePeerGroup) {
                                        if (firewallRule.numPeerGroupIdentifiers ==
                                            HAPArrayCount(firewallRule.peerGroupIdentifiers)) {
                                            HAPLog(&logObject,
                                                   "LAN Rule with more peerGroupIdentifiers than supported.");
                                            isValid = false;
                                        } else {
                                            size_t i = firewallRule.numPeerGroupIdentifiers;
                                            firewallRule.peerGroupIdentifiers[i] =
                                                    (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                            firewallRule.numPeerGroupIdentifiers++;
                                        }
                                    }
                                }
                                callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            }
                            break;
                        }
                        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
                            HAPPlatformWiFiRouterStaticICMPLANFirewallRule firewallRule;
                            HAPRawBufferZero(&firewallRule, sizeof firewallRule);
                            firewallRule.type = kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP;
                            firewallRule.direction = (HAPPlatformWiFiRouterFirewallRuleDirection) direction;
                            firewallRule.peerGroupIdentifiers[0] = (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                            firewallRule.numPeerGroupIdentifiers = 1;
                            firewallRule.icmpTypes[0].icmpProtocol = (HAPPlatformWiFiRouterICMPProtocol) icmpProtocol;
                            if (hasICMPType) {
                                firewallRule.icmpTypes[0].typeValue = (uint8_t) icmpType;
                                firewallRule.icmpTypes[0].typeValueIsSet = true;
                            }
                            firewallRule.numICMPTypes = 1;
                            while (isValid && shouldContinue && (errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                                sqlite_int64 newSortOrder = sqlite3_column_int64(stmt, /* iCol: */ 1);
                                if (newSortOrder != sortOrder) {
                                    didPrefetch = true;
                                    break;
                                }

                                peerGroup = sqlite3_column_int64(stmt, /* iCol: */ 4);

                                icmpProtocol = sqlite3_column_int(stmt, /* iCol: */ 11);

                                hasICMPType = sqlite3_column_type(stmt, /* iCol: */ 12) != SQLITE_NULL;
                                icmpType = sqlite3_column_int(stmt, /* iCol: */ 12);

                                bool isDuplicatePeerGroup = false;
                                for (size_t i = 0; i < firewallRule.numPeerGroupIdentifiers; i++) {
                                    if (peerGroup == firewallRule.peerGroupIdentifiers[i]) {
                                        isDuplicatePeerGroup = true;
                                        break;
                                    }
                                }

                                bool isDuplicateICMPType = false;
                                for (size_t i = 0; i < firewallRule.numICMPTypes; i++) {
                                    const HAPPlatformWiFiRouterICMPType* otherICMPType = &firewallRule.icmpTypes[i];

                                    if (icmpProtocol == otherICMPType->icmpProtocol &&
                                        (hasICMPType ? 1 : 0) == (otherICMPType->typeValueIsSet ? 1 : 0) &&
                                        (!hasICMPType || icmpType == otherICMPType->typeValue)) {
                                        isDuplicateICMPType = true;
                                        break;
                                    }
                                }

                                if (!isDuplicatePeerGroup) {
                                    if (firewallRule.numPeerGroupIdentifiers ==
                                        HAPArrayCount(firewallRule.peerGroupIdentifiers)) {
                                        HAPLog(&logObject, "LAN Rule with more peerGroupIdentifiers than supported.");
                                        isValid = false;
                                    } else {
                                        size_t i = firewallRule.numPeerGroupIdentifiers;
                                        firewallRule.peerGroupIdentifiers[i] =
                                                (HAPPlatformWiFiRouterGroupIdentifier) peerGroup;
                                        firewallRule.numPeerGroupIdentifiers++;
                                    }
                                }
                                if (!isDuplicateICMPType) {
                                    if (firewallRule.numICMPTypes == HAPArrayCount(firewallRule.icmpTypes)) {
                                        HAPLog(&logObject, "WAN Rule with more ICMP type entries than supported.");
                                        isValid = false;
                                    } else {
                                        size_t i = firewallRule.numICMPTypes;
                                        firewallRule.icmpTypes[i].icmpProtocol =
                                                (HAPPlatformWiFiRouterICMPProtocol) icmpProtocol;
                                        if (hasICMPType) {
                                            firewallRule.icmpTypes[i].typeValue = (uint8_t) icmpType;
                                            firewallRule.icmpTypes[i].typeValueIsSet = true;
                                        }
                                        firewallRule.numICMPTypes++;
                                    }
                                }
                            }
                            callback(context, wiFiRouter, clientIdentifier, &firewallRule, &shouldContinue);
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                    if (ip) {
                        sqlite3_free_safe(ip);
                    }
                    if (serviceType) {
                        sqlite3_free_safe(serviceType);
                    }
                    if (errCode == SQLITE_DONE) {
                        break;
                    }
                }
                if (isValid && (errCode == SQLITE_ROW || errCode == SQLITE_DONE)) {
                    err = kHAPError_None;
                }
            }
            HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
            wiFiRouter->synchronization.enumerationNestingLevel--;
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetLANFirewall(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterFirewallType firewallType) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(!wiFiRouter->edits.isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallType);

    HAPError err;
    int errCode;

    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_lan_multicast_bridging`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_lan_static_port`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_lan_dynamic_port`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "DELETE FROM `hap_client_firewalls_lan_static_icmp`\n"
                "WHERE `client` = :client",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "UPDATE `hap_clients` SET\n"
                "    `lan_full_access` = :lan_full_access\n"
                "WHERE `id` = :id",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
            switch (firewallType) {
                case kHAPPlatformWiFiRouterFirewallType_FullAccess: {
                    errCode = errCode ||
                              sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":lan_full_access"), 1);
                    break;
                }
                case kHAPPlatformWiFiRouterFirewallType_Allowlist: {
                    errCode = errCode ||
                              sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, ":lan_full_access"), 0);
                    break;
                }
            }
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    if (!sqlite3_changes(wiFiRouter->db)) {
                        HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                        err = kHAPError_InvalidState;
                    } else {
                        if (firewallType == kHAPPlatformWiFiRouterFirewallType_Allowlist) {
                            wiFiRouter->edits.firewallIndex = 0;
                            wiFiRouter->edits.isEditingLANFirewall = true;
                        }
                        err = kHAPError_None;
                    }
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientAddLANFirewallRule(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        const HAPPlatformWiFiRouterLANFirewallRule* firewallRule_) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(wiFiRouter->edits.isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(firewallRule_);

    HAPError err;
    int errCode;

    switch (*(const HAPPlatformWiFiRouterLANFirewallRuleType*) firewallRule_) {
        case kHAPPlatformWiFiRouterLANFirewallRuleType_MulticastBridging: {
            const HAPPlatformWiFiRouterMulticastBridgingLANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_lan_multicast_bridging` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `direction`,\n"
                    "    `peer_group`,\n"
                    "    `destination_ip`,\n"
                    "    `destination_port`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :direction,\n"
                    "    :peer_group,\n"
                    "    :destination_ip,\n"
                    "    :destination_port\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_None;
            for (size_t i = 0; !err && i < firewallRule->numPeerGroupIdentifiers; i++) {
                err = kHAPError_Unknown;
                errCode = sqlite3_reset(stmt) || sqlite3_clear_bindings(stmt);
                errCode = errCode ||
                          sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                             wiFiRouter->edits.firewallIndex);
                errCode = errCode ||
                          sqlite3_bind_int(
                                  stmt, sqlite3_bind_parameter_index(stmt, ":direction"), firewallRule->direction);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":peer_group"),
                                             firewallRule->peerGroupIdentifiers[i]);
                switch (firewallRule->destinationIP.version) {
                    case kHAPIPAddressVersion_IPv4: {
                        errCode = errCode || sqlite3_bind_blob(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":destination_ip"),
                                                     firewallRule->destinationIP._.ipv4.bytes,
                                                     sizeof firewallRule->destinationIP._.ipv4.bytes,
                                                     SQLITE_STATIC);
                        break;
                    }
                    case kHAPIPAddressVersion_IPv6: {
                        errCode = errCode || sqlite3_bind_blob(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":destination_ip"),
                                                     firewallRule->destinationIP._.ipv6.bytes,
                                                     sizeof firewallRule->destinationIP._.ipv6.bytes,
                                                     SQLITE_STATIC);
                        break;
                    }
                }
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":destination_port"),
                                             firewallRule->destinationPort);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            if (!err) {
                wiFiRouter->edits.firewallIndex++;
                HAPAssert(wiFiRouter->edits.firewallIndex);
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticPort: {
            const HAPPlatformWiFiRouterStaticPortLANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_lan_static_port` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `direction`,\n"
                    "    `transport_protocol`,\n"
                    "    `peer_group`,\n"
                    "    `destination_port_start`,\n"
                    "    `destination_port_end`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :direction,\n"
                    "    :transport_protocol,\n"
                    "    :peer_group,\n"
                    "    :destination_port_start,\n"
                    "    :destination_port_end\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_None;
            for (size_t i = 0; !err && i < firewallRule->numPeerGroupIdentifiers; i++) {
                err = kHAPError_Unknown;
                errCode = sqlite3_reset(stmt) || sqlite3_clear_bindings(stmt);
                errCode = errCode ||
                          sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                             wiFiRouter->edits.firewallIndex);
                errCode = errCode ||
                          sqlite3_bind_int(
                                  stmt, sqlite3_bind_parameter_index(stmt, ":direction"), firewallRule->direction);
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":transport_protocol"),
                                             firewallRule->transportProtocol);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":peer_group"),
                                             firewallRule->peerGroupIdentifiers[i]);
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":destination_port_start"),
                                             firewallRule->destinationPortRange.startPort);
                if (firewallRule->destinationPortRange.endPort != firewallRule->destinationPortRange.startPort) {
                    errCode = errCode || sqlite3_bind_int(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":destination_port_end"),
                                                 firewallRule->destinationPortRange.endPort);
                }
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            if (!err) {
                wiFiRouter->edits.firewallIndex++;
                HAPAssert(wiFiRouter->edits.firewallIndex);
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_DynamicPort: {
            const HAPPlatformWiFiRouterDynamicPortLANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_lan_dynamic_port` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `direction`,\n"
                    "    `transport_protocol`,\n"
                    "    `peer_group`,\n"
                    "    `advertisement_protocol`,\n"
                    "    `service_type`,\n"
                    "    `advertisement_only`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :direction,\n"
                    "    :transport_protocol,\n"
                    "    :peer_group,\n"
                    "    :advertisement_protocol,\n"
                    "    :service_type,\n"
                    "    :advertisement_only\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_None;
            for (size_t i = 0; !err && i < firewallRule->numPeerGroupIdentifiers; i++) {
                err = kHAPError_Unknown;
                errCode = sqlite3_reset(stmt) || sqlite3_clear_bindings(stmt);
                errCode = errCode ||
                          sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                             wiFiRouter->edits.firewallIndex);
                errCode = errCode ||
                          sqlite3_bind_int(
                                  stmt, sqlite3_bind_parameter_index(stmt, ":direction"), firewallRule->direction);
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":transport_protocol"),
                                             firewallRule->transportProtocol);
                errCode = errCode || sqlite3_bind_int64(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":peer_group"),
                                             firewallRule->peerGroupIdentifiers[i]);
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":advertisement_protocol"),
                                             firewallRule->serviceType.advertisementProtocol);
                switch (firewallRule->serviceType.advertisementProtocol) {
                    case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_DNSSD: {
                        errCode = errCode || sqlite3_bind_text(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":service_type"),
                                                     firewallRule->serviceType._.dns_sd.serviceType,
                                                     /* nByte: */ -1,
                                                     SQLITE_STATIC);
                        break;
                    }
                    case kHAPPlatformWiFiRouterDynamicPortAdvertisementProtocol_SSDP: {
                        errCode = errCode || sqlite3_bind_text(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":service_type"),
                                                     firewallRule->serviceType._.ssdp.serviceTypeURI,
                                                     /* nByte: */ -1,
                                                     SQLITE_STATIC);
                        break;
                    }
                }
                errCode = errCode || sqlite3_bind_int(
                                             stmt,
                                             sqlite3_bind_parameter_index(stmt, ":advertisement_only"),
                                             firewallRule->advertisementOnly ? 1 : 0);
                if (!errCode) {
                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
            if (!err) {
                wiFiRouter->edits.firewallIndex++;
                HAPAssert(wiFiRouter->edits.firewallIndex);
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        case kHAPPlatformWiFiRouterLANFirewallRuleType_StaticICMP: {
            const HAPPlatformWiFiRouterStaticICMPLANFirewallRule* firewallRule = firewallRule_;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "INSERT INTO `hap_client_firewalls_lan_static_icmp` (\n"
                    "    `client`,\n"
                    "    `sort_order`,\n"
                    "    `direction`,\n"
                    "    `peer_group`,\n"
                    "    `icmp_protocol`,\n"
                    "    `icmp_type`\n"
                    ") VALUES (\n"
                    "    :client,\n"
                    "    :sort_order,\n"
                    "    :direction,\n"
                    "    :peer_group,\n"
                    "    :icmp_protocol,\n"
                    "    :icmp_type\n"
                    ")",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            err = kHAPError_None;
            for (size_t i = 0; !err && i < firewallRule->numPeerGroupIdentifiers; i++) {
                for (size_t j = 0; !err && j < firewallRule->numICMPTypes; j++) {
                    err = kHAPError_Unknown;
                    errCode = sqlite3_reset(stmt) || sqlite3_clear_bindings(stmt);
                    errCode = errCode ||
                              sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":client"), clientIdentifier);
                    errCode = errCode || sqlite3_bind_int64(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":sort_order"),
                                                 wiFiRouter->edits.firewallIndex);
                    errCode = errCode ||
                              sqlite3_bind_int(
                                      stmt, sqlite3_bind_parameter_index(stmt, ":direction"), firewallRule->direction);
                    errCode = errCode || sqlite3_bind_int64(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":peer_group"),
                                                 firewallRule->peerGroupIdentifiers[i]);
                    errCode = errCode || sqlite3_bind_int(
                                                 stmt,
                                                 sqlite3_bind_parameter_index(stmt, ":icmp_protocol"),
                                                 firewallRule->icmpTypes[j].icmpProtocol);
                    if (firewallRule->icmpTypes[j].typeValueIsSet) {
                        errCode = errCode || sqlite3_bind_int(
                                                     stmt,
                                                     sqlite3_bind_parameter_index(stmt, ":icmp_type"),
                                                     firewallRule->icmpTypes[j].typeValue);
                    }
                    if (!errCode) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            err = kHAPError_None;
                        }
                    }
                }
            }
            if (!err) {
                wiFiRouter->edits.firewallIndex++;
                HAPAssert(wiFiRouter->edits.firewallIndex);
            }
            sqlite3_finalize_safe(stmt);
        }
            return err;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientFinalizeLANFirewallRules(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(wiFiRouter->edits.isEditingLANFirewall);
    HAPPrecondition(clientIdentifier);

    wiFiRouter->edits.firewallIndex = 0;
    wiFiRouter->edits.isEditingLANFirewall = false;
    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool ShouldIncludeClientStatusForIdentifiers(
        const HAPPlatformWiFiRouterClientStatus* clientStatus,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers) {
    HAPPrecondition(clientStatus);
    HAPPrecondition(statusIdentifiers);

    for (size_t i = 0; i < numStatusIdentifiers; i++) {
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifier = &statusIdentifiers[i];
        switch (statusIdentifier->format) {
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_Client: {
                if (clientStatus->clientIdentifier == statusIdentifier->clientIdentifier._) {
                    return true;
                }
                break;
            }
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_MACAddress: {
                if (HAPMACAddressAreEqual(&statusIdentifier->macAddress._, clientStatus->macAddress)) {
                    return true;
                }
                break;
            }
            case kHAPPlatformWiFiRouterClientStatusIdentifierFormat_IPAddress: {
                for (size_t j = 0; j < clientStatus->numIPAddresses; j++) {
                    if (HAPIPAddressAreEqual(&statusIdentifier->ipAddress._, &clientStatus->ipAddresses[j])) {
                        return true;
                    }
                }
                break;
            }
        }
    }
    return false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterGetClientStatusForIdentifiers(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPPlatformWiFiRouterClientStatusIdentifier* statusIdentifiers,
        size_t numStatusIdentifiers,
        HAPPlatformWiFiRouterGetClientStatusForIdentifiersCallback callback,
        void* _Nullable context) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(callback);

    wiFiRouter->synchronization.enumerationNestingLevel++;
    HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (!wiFiRouter->connections[i].isActive) {
            continue;
        }

        HAPPlatformWiFiRouterClientStatus clientStatus = {
            .clientIdentifier = wiFiRouter->connections[i].clientIdentifier,
            .macAddress = &wiFiRouter->connections[i].macAddress,
            .ipAddresses = wiFiRouter->connections[i].ipAddresses,
            .numIPAddresses = wiFiRouter->connections[i].numIPAddresses,
            .name = HAPStringGetNumBytes(wiFiRouter->connections[i].name) ? wiFiRouter->connections[i].name : NULL,
            .rssi = { .isDefined = wiFiRouter->connections[i].rssi.isDefined,
                      .value = wiFiRouter->connections[i].rssi.value }
        };

        if (!ShouldIncludeClientStatusForIdentifiers(&clientStatus, statusIdentifiers, numStatusIdentifiers)) {
            continue;
        }

        callback(context, wiFiRouter, &clientStatus, &shouldContinue);
    }

    HAPAssert(wiFiRouter->synchronization.enumerationNestingLevel);
    wiFiRouter->synchronization.enumerationNestingLevel--;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterConnectClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPWiFiWPAPersonalCredential* _Nullable credential,
        const HAPMACAddress* macAddress,
        const char* _Nullable name,
        HAPIPAddress* _Nullable ipAddress) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterIsConfigurationAccessAvailable(wiFiRouter));
    HAPPrecondition(macAddress);

    HAPError err;
    int errCode;

    err = HAPPlatformWiFiRouterAcquireSharedConfigurationAccess(wiFiRouter);
    HAPAssert(!err);

    HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;
    if (credential) {
        // Check if a matching network client profile configuration exists.
        switch (credential->type) {
            case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                sqlite3_stmt* stmt;
                errCode = sqlite3_prepare_v2(
                        wiFiRouter->db,
                        "SELECT\n"
                        "    `id`\n" // 0.
                        "FROM `hap_clients`\n"
                        "WHERE `credential_passphrase` = :credential_passphrase\n"
                        "LIMIT 1",
                        /* nByte: */ -1,
                        &stmt,
                        /* pzTail: */ NULL);
                if (errCode) {
                    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
                    return kHAPError_Unknown;
                }

                err = kHAPError_Unknown;
                {
                    errCode = sqlite3_bind_text(
                            stmt,
                            sqlite3_bind_parameter_index(stmt, ":credential_passphrase"),
                            credential->_.passphrase.stringValue,
                            /* nBytes: */ -1,
                            SQLITE_STATIC);
                    if (!errCode) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            HAPLog(&logObject, "Network client not found for given WPA Personal passphrase.");
                            err = kHAPError_None;
                        } else if (errCode == SQLITE_ROW) {
                            sqlite_int64 client = sqlite3_column_int64(stmt, /* iCol: */ 0);

                            clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) client;
                            errCode = sqlite3_step(stmt);
                            if (errCode == SQLITE_DONE) {
                                err = kHAPError_None;
                            }
                        }
                    }
                }
                sqlite3_finalize_safe(stmt);
                if (err) {
                    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
                    return err;
                }
                break;
            }
            case kHAPWiFiWPAPersonalCredentialType_PSK: {
                sqlite3_stmt* stmt;
                errCode = sqlite3_prepare_v2(
                        wiFiRouter->db,
                        "SELECT\n"
                        "    `id`\n"
                        "FROM `hap_clients`\n"
                        "WHERE `credential_psk` = :credential_psk\n"
                        "LIMIT 1",
                        /* nByte: */ -1,
                        &stmt,
                        /* pzTail: */ NULL);
                if (errCode) {
                    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
                    return kHAPError_Unknown;
                }

                err = kHAPError_Unknown;
                {
                    errCode = sqlite3_bind_blob(
                            stmt,
                            sqlite3_bind_parameter_index(stmt, ":credential_psk"),
                            credential->_.psk.bytes,
                            sizeof credential->_.psk.bytes,
                            SQLITE_STATIC);
                    if (!errCode) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            HAPLog(&logObject, "Network client not found for given WPA PSK.");
                            err = kHAPError_None;
                        } else if (errCode == SQLITE_ROW) {
                            sqlite_int64 client = sqlite3_column_int64(stmt, /* iCol: */ 0);

                            clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) client;
                            errCode = sqlite3_step(stmt);
                            if (errCode == SQLITE_DONE) {
                                err = kHAPError_None;
                            }
                        }
                    }
                }
                sqlite3_finalize_safe(stmt);
                if (err) {
                    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
                    return err;
                }
                break;
            }
        }
    }
    if (!clientIdentifier) {
        // Check if a network client profile configuration based on MAC address exists.
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "SELECT\n"
                "    `id`\n"
                "FROM `hap_clients`\n"
                "WHERE `credential_mac` = :credential_mac\n"
                "LIMIT 1",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_blob(
                    stmt,
                    sqlite3_bind_parameter_index(stmt, ":credential_mac"),
                    macAddress->bytes,
                    sizeof macAddress->bytes,
                    SQLITE_STATIC);
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    HAPLog(&logObject, "Network client not found for given MAC address.");
                    err = kHAPError_None;
                } else if (errCode == SQLITE_ROW) {
                    sqlite_int64 client = sqlite3_column_int64(stmt, /* iCol: */ 0);

                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) client;

                    errCode = sqlite3_step(stmt);
                    if (errCode == SQLITE_DONE) {
                        err = kHAPError_None;
                    }
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
            return kHAPError_Unknown;
        }
    }

    // Add connection.
    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (wiFiRouter->connections[i].isActive) {
            if (HAPRawBufferAreEqual(
                        wiFiRouter->connections[i].macAddress.bytes, macAddress->bytes, sizeof macAddress->bytes)) {
                HAPLog(&logObject, "Network client with the given MAC address is already connected.");
                HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
                return kHAPError_InvalidState;
            }

            continue;
        }

        // Basic network client information.
        wiFiRouter->connections[i].isActive = true;
        wiFiRouter->connections[i].clientIdentifier = clientIdentifier;
        wiFiRouter->connections[i].macAddress = *macAddress;

        // Assign IP addresses.
        size_t numIPAddresses = 0;
        HAPAssert(numIPAddresses < HAPArrayCount(wiFiRouter->connections[i].ipAddresses));
        wiFiRouter->connections[i].ipAddresses[numIPAddresses].version = kHAPIPAddressVersion_IPv4;
        HAPAssert(i <= UINT8_MAX - 2);
        wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[0] = 10;
        wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[1] = 0;
        wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[2] = 1;
        wiFiRouter->connections[i].ipAddresses[numIPAddresses]._.ipv4.bytes[3] = (uint8_t)(i + 3);
        numIPAddresses++;
        wiFiRouter->connections[i].numIPAddresses = numIPAddresses;
        if (ipAddress) {
            HAPRawBufferCopyBytes(HAPNonnull(ipAddress), &wiFiRouter->connections[i].ipAddresses[0], sizeof *ipAddress);
        }

        // Network client name.
        if (name) {
            size_t numNameBytes = HAPStringGetNumBytes(HAPNonnull(name));
            if (numNameBytes >= sizeof wiFiRouter->connections[i].name) {
                HAPLog(&logObject, "Network client has too long name. Not keeping track of name.");
            } else {
                HAPRawBufferCopyBytes(wiFiRouter->connections[i].name, HAPNonnull(name), numNameBytes + 1);
            }
        }

        // Generate RSSI.
        wiFiRouter->connections[i].rssi.isDefined = true;
        do {
            HAPPlatformRandomNumberFill(
                    &wiFiRouter->connections[i].rssi.value, sizeof wiFiRouter->connections[i].rssi.value);
        } while (wiFiRouter->connections[i].rssi.value > 0);

        HAPLogInfo(
                &logObject,
                "Network client %02X:%02X:%02X:%02X:%02X:%02X connected (network client identifier %lu).",
                macAddress->bytes[0],
                macAddress->bytes[1],
                macAddress->bytes[2],
                macAddress->bytes[3],
                macAddress->bytes[4],
                macAddress->bytes[5],
                (unsigned long) clientIdentifier);
        HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
        return kHAPError_None;
    }

    HAPLog(&logObject, "Reached limit of concurrently connected clients.");
    HAPPlatformWiFiRouterReleaseSharedConfigurationAccess(wiFiRouter);
    return kHAPError_OutOfResources;
}

void HAPPlatformWiFiRouterDisconnectClient(HAPPlatformWiFiRouterRef wiFiRouter, const HAPMACAddress* macAddress) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(macAddress);

    for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
        if (!wiFiRouter->connections[i].isActive) {
            continue;
        }
        if (!HAPRawBufferAreEqual(
                    wiFiRouter->connections[i].macAddress.bytes, macAddress->bytes, sizeof macAddress->bytes)) {
            continue;
        }

        HAPLogInfo(
                &logObject,
                "Network client %02X:%02X:%02X:%02X:%02X:%02X disconnected.",
                macAddress->bytes[0],
                macAddress->bytes[1],
                macAddress->bytes[2],
                macAddress->bytes[3],
                macAddress->bytes[4],
                macAddress->bytes[5]);
        HAPRawBufferZero(&wiFiRouter->connections[i], sizeof wiFiRouter->connections[i]);
        return;
    }

    HAPLog(&logObject,
           "Network client %02X:%02X:%02X:%02X:%02X:%02X was not found in connection list.",
           macAddress->bytes[0],
           macAddress->bytes[1],
           macAddress->bytes[2],
           macAddress->bytes[3],
           macAddress->bytes[4],
           macAddress->bytes[5]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientGetAccessViolationMetadata(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterAccessViolationMetadata* violationMetadata) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasSharedConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(clientIdentifier);
    HAPPrecondition(violationMetadata);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "SELECT\n"
            "    `last_reset_timestamp`,\n"    // 0.
            "    `last_violation_timestamp`\n" // 1.
            "FROM `hap_clients`\n"
            "WHERE `id` = :id\n"
            "LIMIT 1",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                err = kHAPError_InvalidState;
            } else if (errCode == SQLITE_ROW) {
                bool hasLastResetTimestamp = sqlite3_column_type(stmt, /* iCol: */ 0) != SQLITE_NULL;
                sqlite3_int64 lastResetTimestamp = sqlite3_column_int64(stmt, /* iCol: */ 0);

                bool hasLastViolationTimestamp = sqlite3_column_type(stmt, /* iCol: */ 1) != SQLITE_NULL;
                sqlite3_int64 lastViolationTimestamp = sqlite3_column_int64(stmt, /* iCol: */ 1);

                HAPRawBufferZero(violationMetadata, sizeof *violationMetadata);
                if (hasLastResetTimestamp) {
                    violationMetadata->lastResetTimestamp = (uint64_t) lastResetTimestamp;
                    violationMetadata->wasReset = true;
                }
                if (hasLastViolationTimestamp) {
                    violationMetadata->lastViolationTimestamp = (uint64_t) lastViolationTimestamp;
                    violationMetadata->hasViolations = true;
                }
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientResetAccessViolations(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(HAPPlatformWiFiRouterHasExclusiveConfigurationAccess(wiFiRouter));
    HAPPrecondition(!wiFiRouter->edits.isEditing);
    HAPPrecondition(!wiFiRouter->synchronization.enumerationNestingLevel);
    HAPPrecondition(clientIdentifier);

    HAPError err;
    int errCode;

    sqlite3_stmt* stmt;
    errCode = sqlite3_prepare_v2(
            wiFiRouter->db,
            "UPDATE `hap_clients` SET\n"
            "    `last_reset_timestamp` = strftime('%s', 'now'),\n"
            "    `last_violation_timestamp` = NULL\n"
            "WHERE `id` = :id",
            /* nByte: */ -1,
            &stmt,
            /* pzTail: */ NULL);
    if (errCode) {
        return kHAPError_Unknown;
    }

    err = kHAPError_Unknown;
    {
        errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
        if (!errCode) {
            errCode = sqlite3_step(stmt);
            if (errCode == SQLITE_DONE) {
                if (!sqlite3_changes(wiFiRouter->db)) {
                    HAPLog(&logObject, "Network client profile %lu not found.", (unsigned long) clientIdentifier);
                }
                err = kHAPError_None;
            }
        }
    }
    sqlite3_finalize_safe(stmt);
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientRecordAccessViolation(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterTimestamp timestamp) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(clientIdentifier);

    HAPError err;
    int errCode;

    HAPLogInfo(
            &logObject,
            "Recording network access violation for network client profile %lu (timestamp %lld).",
            (unsigned long) clientIdentifier,
            (long long) timestamp);

    bool hadViolations = false;
    {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "UPDATE `hap_clients` SET\n"
                "    `last_violation_timestamp` = :last_violation_timestamp\n"
                "WHERE\n"
                "    `id` = :id AND\n"
                "    `last_violation_timestamp` IS NULL",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
            errCode = errCode || sqlite3_bind_int64(
                                         stmt,
                                         sqlite3_bind_parameter_index(stmt, ":last_violation_timestamp"),
                                         (sqlite3_int64) HAPMin(timestamp, INT64_MAX));
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    hadViolations = sqlite3_changes(wiFiRouter->db) == 0;
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }
    if (hadViolations) {
        sqlite3_stmt* stmt;
        errCode = sqlite3_prepare_v2(
                wiFiRouter->db,
                "UPDATE `hap_clients` SET\n"
                "    `last_violation_timestamp` = :last_violation_timestamp\n"
                "WHERE\n"
                "    `id` = :id AND\n"
                "    `last_reset_timestamp` < :last_violation_timestamp",
                /* nByte: */ -1,
                &stmt,
                /* pzTail: */ NULL);
        if (errCode) {
            return kHAPError_Unknown;
        }

        err = kHAPError_Unknown;
        {
            errCode = sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":id"), clientIdentifier);
            errCode = errCode || sqlite3_bind_int64(
                                         stmt,
                                         sqlite3_bind_parameter_index(stmt, ":last_violation_timestamp"),
                                         (sqlite3_int64) HAPMin(timestamp, INT64_MAX));
            if (!errCode) {
                errCode = sqlite3_step(stmt);
                if (errCode == SQLITE_DONE) {
                    err = kHAPError_None;
                }
            }
        }
        sqlite3_finalize_safe(stmt);
        if (err) {
            return err;
        }
    }

    if (!hadViolations && wiFiRouter->delegate.handleAccessViolationMetadataChanged) {
        wiFiRouter->delegate.handleAccessViolationMetadataChanged(
                wiFiRouter, clientIdentifier, wiFiRouter->delegate.context);
    }
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_RESULT_USE_CHECK
static bool SatelliteStatusIsValid(HAPPlatformWiFiRouterSatelliteStatus value) {
    switch (value) {
        case kHAPPlatformWiFiRouterSatelliteStatus_Unknown:
        case kHAPPlatformWiFiRouterSatelliteStatus_Connected:
        case kHAPPlatformWiFiRouterSatelliteStatus_NotConnected: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static const char* GetSatelliteStatusDescription(HAPPlatformWiFiRouterSatelliteStatus value) {
    switch (value) {
        case kHAPPlatformWiFiRouterSatelliteStatus_Unknown:
            return "unknown";
        case kHAPPlatformWiFiRouterSatelliteStatus_Connected:
            return "connected";
        case kHAPPlatformWiFiRouterSatelliteStatus_NotConnected:
            return "not connected";
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPPlatformWiFiRouterSatelliteStatus
        HAPPlatformWiFiRouterGetSatelliteStatus(HAPPlatformWiFiRouterRef wiFiRouter, size_t satelliteIndex) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(satelliteIndex < HAPArrayCount(wiFiRouter->satellites));

    HAPPlatformWiFiRouterSatelliteStatus status = wiFiRouter->satellites[satelliteIndex].satelliteStatus;

    HAPLogInfo(
            &logObject,
            "Getting status of simulated Wi-Fi satellite accessory %zu: %s.",
            satelliteIndex,
            GetSatelliteStatusDescription(status));

    return status;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterSetSatelliteStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        size_t satelliteIndex,
        HAPPlatformWiFiRouterSatelliteStatus status) {
    HAPPrecondition(wiFiRouter);
    HAPPrecondition(SatelliteStatusIsValid(status));

    if (satelliteIndex >= HAPArrayCount(wiFiRouter->satellites)) {
        HAPLogError(&logObject, "No simulated Wi-Fi satellite accessory with index %zu exists.", satelliteIndex);
        return kHAPError_InvalidState;
    }

    HAPLogInfo(
            &logObject,
            "Setting status of simulated Wi-Fi satellite accessory %zu: %s.",
            satelliteIndex,
            GetSatelliteStatusDescription(status));

    if (wiFiRouter->satellites[satelliteIndex].satelliteStatus != status) {
        wiFiRouter->satellites[satelliteIndex].satelliteStatus = status;
        if (wiFiRouter->delegate.handleSatelliteStatusChanged) {
            wiFiRouter->delegate.handleSatelliteStatusChanged(wiFiRouter, satelliteIndex, wiFiRouter->delegate.context);
        }
    }

    return kHAPError_None;
}

#endif
