#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

class Database {
public:
    static Database& getInstance();

    // Connects using CHARITY_DB_HOST / CHARITY_DB_PORT / CHARITY_DB_NAME /
    // CHARITY_DB_USER / CHARITY_DB_PASSWORD from the environment when set,
    // falling back to the localhost/charity_fund/charity_user defaults from
    // the README's setup steps. This lets anyone who already has a
    // differently-configured PostgreSQL instance point the app at it
    // without editing and recompiling the source.
    bool connect();

    void disconnect();
    bool isConnected() const;

    QSqlDatabase& getConnection();
    QString getLastError() const { return lastError_; }

    // Actual connection parameters in effect (defaults, or overridden via
    // CHARITY_DB_* env vars — see connect()). Used for diagnostics/status
    // display so those never show stale hardcoded values when the app is
    // pointed at a non-default database.
    QString getHost() const { return connection_.hostName(); }
    int getPort() const { return connection_.port(); }
    QString getDatabaseName() const { return connection_.databaseName(); }
    QString getUserName() const { return connection_.userName(); }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

private:
    Database();
    ~Database();
    // Creates any tables/indexes/triggers/views that don't exist yet
    // (idempotent — see resources/../migrations.sql). Run automatically
    // on every successful connect() so a fresh PostgreSQL database (with
    // just the role/db created) gets a working schema without a manual
    // `psql -f setup_db.sql` step.
    bool migrate();

    QSqlDatabase connection_;
    QString lastError_;
    bool connected_;
};

#endif // DATABASE_H
