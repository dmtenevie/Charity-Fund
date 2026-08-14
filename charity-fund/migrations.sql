-- Idempotent schema migration, embedded into the binary and run
-- automatically on every startup (see Database::migrate()). Safe to run
-- against an already-populated database — never drops or truncates
-- anything, only creates what's missing.
--
-- This is intentionally separate from setup_db.sql, which additionally
-- seeds demo data and starts with destructive DROP TABLE statements —
-- keep that one for developers who explicitly want a full reset.

CREATE TABLE IF NOT EXISTS donors (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    phone VARCHAR(50),
    address TEXT,
    registration_date DATE NOT NULL DEFAULT CURRENT_DATE,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Normalize any pre-existing empty-string emails to NULL. The donors.email
-- column is UNIQUE but optional, and an empty string is a real (non-NULL)
-- value as far as that constraint is concerned — so a second donor left
-- without an email would collide with an earlier '' row and fail to save.
-- The Repository now binds NULL instead of "" for a blank email going
-- forward; this just cleans up rows already written the old way.
UPDATE donors SET email = NULL WHERE email = '';

CREATE TABLE IF NOT EXISTS projects (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    goal_amount DECIMAL(12, 2) NOT NULL DEFAULT 0,
    current_amount DECIMAL(12, 2) NOT NULL DEFAULT 0,
    start_date DATE NOT NULL,
    end_date DATE,
    status VARCHAR(50) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CHECK (goal_amount >= 0),
    CHECK (current_amount >= 0)
);

-- Projects used to also require current_amount <= goal_amount, but that
-- rejects perfectly normal over-funding (donors keep giving after a project
-- hits its goal) and made adding a donation fail outright once a project's
-- total passed its target. Drop it from any database that still has it from
-- before this fix — CREATE TABLE IF NOT EXISTS above won't touch an
-- already-existing projects table, so this has to be done explicitly and by
-- constraint definition (not by a fixed name), since Postgres auto-names
-- unnamed CHECK constraints and the exact name depends on table history.
DO $$
DECLARE
    constraint_name TEXT;
BEGIN
    SELECT con.conname INTO constraint_name
    FROM pg_constraint con
    JOIN pg_class rel ON rel.oid = con.conrelid
    WHERE rel.relname = 'projects'
      AND con.contype = 'c'
      AND pg_get_constraintdef(con.oid) = 'CHECK ((current_amount <= goal_amount))';

    IF constraint_name IS NOT NULL THEN
        EXECUTE format('ALTER TABLE projects DROP CONSTRAINT %I', constraint_name);
    END IF;
END $$;

CREATE TABLE IF NOT EXISTS donations (
    id SERIAL PRIMARY KEY,
    donor_id INTEGER NOT NULL REFERENCES donors(id) ON DELETE CASCADE,
    project_id INTEGER REFERENCES projects(id) ON DELETE SET NULL,
    amount DECIMAL(12, 2) NOT NULL,
    donation_date DATE NOT NULL DEFAULT CURRENT_DATE,
    payment_method VARCHAR(50) DEFAULT 'cash',
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CHECK (amount > 0)
);

CREATE TABLE IF NOT EXISTS beneficiaries (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    contact VARCHAR(255),
    project_id INTEGER REFERENCES projects(id) ON DELETE SET NULL,
    description TEXT,
    assistance_type VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_donations_donor ON donations(donor_id);
CREATE INDEX IF NOT EXISTS idx_donations_project ON donations(project_id);
CREATE INDEX IF NOT EXISTS idx_donations_date ON donations(donation_date);
CREATE INDEX IF NOT EXISTS idx_beneficiaries_project ON beneficiaries(project_id);
CREATE INDEX IF NOT EXISTS idx_projects_status ON projects(status);

CREATE OR REPLACE FUNCTION update_project_amount()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.project_id IS NOT NULL THEN
        UPDATE projects
        SET current_amount = current_amount + NEW.amount
        WHERE id = NEW.project_id;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS donation_insert_trigger ON donations;
CREATE TRIGGER donation_insert_trigger
AFTER INSERT ON donations
FOR EACH ROW
EXECUTE FUNCTION update_project_amount();

CREATE OR REPLACE FUNCTION revert_project_amount()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.project_id IS NOT NULL THEN
        UPDATE projects
        SET current_amount = current_amount - OLD.amount
        WHERE id = OLD.project_id;
    END IF;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS donation_delete_trigger ON donations;
CREATE TRIGGER donation_delete_trigger
AFTER DELETE ON donations
FOR EACH ROW
EXECUTE FUNCTION revert_project_amount();

CREATE OR REPLACE FUNCTION adjust_project_amount_on_update()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.project_id IS NOT NULL THEN
        UPDATE projects
        SET current_amount = current_amount - OLD.amount
        WHERE id = OLD.project_id;
    END IF;
    IF NEW.project_id IS NOT NULL THEN
        UPDATE projects
        SET current_amount = current_amount + NEW.amount
        WHERE id = NEW.project_id;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS donation_update_trigger ON donations;
CREATE TRIGGER donation_update_trigger
AFTER UPDATE ON donations
FOR EACH ROW
EXECUTE FUNCTION adjust_project_amount_on_update();

CREATE OR REPLACE VIEW donation_summary AS
SELECT
    d.id,
    don.name AS donor_name,
    p.name AS project_name,
    d.amount,
    d.donation_date,
    d.payment_method,
    d.notes
FROM donations d
LEFT JOIN donors don ON d.donor_id = don.id
LEFT JOIN projects p ON d.project_id = p.id
ORDER BY d.donation_date DESC;

CREATE OR REPLACE VIEW donor_statistics AS
SELECT
    don.id,
    don.name,
    COUNT(d.id) AS total_donations,
    COALESCE(SUM(d.amount), 0) AS total_amount,
    MAX(d.donation_date) AS last_donation_date
FROM donors don
LEFT JOIN donations d ON don.id = d.donor_id
GROUP BY don.id, don.name
ORDER BY total_amount DESC;

COMMENT ON TABLE donors IS 'Таблиця благодійників (донорів)';
COMMENT ON TABLE projects IS 'Таблиця благодійних проектів';
COMMENT ON TABLE donations IS 'Таблиця пожертв';
COMMENT ON TABLE beneficiaries IS 'Таблиця отримувачів допомоги';
