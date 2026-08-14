DROP TABLE IF EXISTS donations CASCADE;
DROP TABLE IF EXISTS beneficiaries CASCADE;
DROP TABLE IF EXISTS projects CASCADE;
DROP TABLE IF EXISTS donors CASCADE;

CREATE TABLE donors (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    phone VARCHAR(50),
    address TEXT,
    registration_date DATE NOT NULL DEFAULT CURRENT_DATE,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE projects (
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

CREATE TABLE donations (
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

CREATE TABLE beneficiaries (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    contact VARCHAR(255),
    project_id INTEGER REFERENCES projects(id) ON DELETE SET NULL,
    description TEXT,
    assistance_type VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_donations_donor ON donations(donor_id);
CREATE INDEX idx_donations_project ON donations(project_id);
CREATE INDEX idx_donations_date ON donations(donation_date);
CREATE INDEX idx_beneficiaries_project ON beneficiaries(project_id);
CREATE INDEX idx_projects_status ON projects(status);

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

CREATE TRIGGER donation_update_trigger
AFTER UPDATE ON donations
FOR EACH ROW
EXECUTE FUNCTION adjust_project_amount_on_update();

INSERT INTO donors (name, email, phone, address, notes) VALUES
    ('Іван Петренко', 'ivan.petrenko@example.com', '+380501234567', 'м. Київ, вул. Хрещатик, 1', 'Постійний донор, щомісячний переказ'),
    ('Марія Коваленко', 'maria.kovalenko@example.com', '+380672345678', 'м. Львів, пр. Свободи, 10', NULL),
    ('Олександр Шевченко', 'oleksandr.shevchenko@example.com', '+380933456789', 'м. Одеса, вул. Дерибасівська, 5', 'Перший внесок 2026'),
    ('ТОВ «Агротех»', 'contact@agrotech.ua', '+380442345678', 'м. Київ, вул. Промислова, 22', 'Корпоративний донор'),
    ('Наталія Мельник', 'natalia.melnyk@example.com', '+380631112233', 'м. Дніпро, пр. Гагаріна, 15', NULL),
    ('Андрій Ткачук', 'andrii.tkachuk@example.com', '+380971234567', 'м. Харків, вул. Сумська, 8', NULL),
    ('Софія Бондаренко', 'sofia.bondarenko@example.com', '+380507654321', 'м. Одеса, вул. Пушкінська, 3', 'Знайшла фонд через соцмережі'),
    ('Дмитро Кравченко', 'dmytro.kravchenko@example.com', '+380667778899', 'м. Запоріжжя, пр. Соборний, 40', NULL),
    ('Оксана Іваненко', 'oksana.ivanenko@example.com', '+380935556677', 'м. Вінниця, вул. Соборна, 12', NULL),
    ('Сергій Бойко', 'serhii.boiko@example.com', '+380441239988', 'м. Київ, вул. Велика Васильківська, 55', 'Постійний донор'),
    ('Тетяна Савченко', 'tetiana.savchenko@example.com', '+380672224455', 'м. Львів, вул. Личаківська, 20', NULL),
    ('Віктор Лисенко', 'viktor.lysenko@example.com', '+380933337788', 'м. Полтава, вул. Соборності, 7', NULL),
    ('Юлія Мороз', 'yuliia.moroz@example.com', '+380504445566', 'м. Чернігів, пр. Миру, 18', NULL),
    ('ТОВ «Будсервіс»', 'info@budservis.ua', '+380445678901', 'м. Київ, вул. Antonovycha, 90', 'Корпоративний донор, щоквартально'),
    ('Роман Гончаренко', 'roman.honcharenko@example.com', '+380663213214', 'м. Рівне, вул. Соборна, 5', NULL),
    ('Ірина Захарченко', 'iryna.zakharchenko@example.com', '+380971112200', 'м. Тернопіль, вул. Руська, 11', NULL),
    ('Максим Поліщук', 'maksym.polishchuk@example.com', '+380509998877', 'м. Луцьк, пр. Волі, 25', NULL),
    ('Катерина Романенко', 'kateryna.romanenko@example.com', '+380635554433', 'м. Ужгород, пл. Театральна, 2', NULL),
    ('Павло Кузьменко', 'pavlo.kuzmenko@example.com', '+380931230045', 'м. Черкаси, б-р Шевченка, 60', NULL),
    ('Анна Ковальчук', 'anna.kovalchuk@example.com', '+380442221100', 'м. Суми, вул. Соборна, 30', 'Волонтерить у фонді'),
    ('ФОП Клименко О.В.', 'klymenko.fop@example.com', '+380507771122', 'м. Житомир, вул. Київська, 45', 'Малий бізнес-донор'),
    ('Богдан Литвиненко', 'bohdan.lytvynenko@example.com', '+380663332211', 'м. Миколаїв, пр. Центральний, 33', NULL),
    ('Валентина Сидоренко', 'valentyna.sydorenko@example.com', '+380975556644', 'м. Кропивницький, вул. Дворцова, 14', NULL),
    ('Микола Данилевич', 'mykola.danylevych@example.com', '+380508889900', 'м. Івано-Франківськ, вул. Незалежності, 9', NULL);

INSERT INTO projects (name, description, goal_amount, start_date, end_date, status) VALUES
    ('Допомога дітям-сиротам', 'Програма підтримки дітей з дитячих будинків', 100000.00, '2026-01-01', '2026-12-31', 'active'),
    ('Медична допомога', 'Закупівля медичного обладнання для лікарень', 250000.00, '2026-02-01', '2026-11-30', 'active'),
    ('Освітні програми', 'Стипендії для талановитої молоді', 50000.00, '2026-01-15', '2026-06-30', 'completed'),
    ('Зимова підтримка родин', 'Продуктові набори та тепле вбрання для малозабезпечених родин', 80000.00, '2026-01-01', '2026-03-31', 'active');

INSERT INTO donations (donor_id, project_id, amount, donation_date, payment_method, notes) VALUES
    -- Допомога дітям-сиротам (goal 100 000, ~68 000 зібрано)
    (1, 1, 5000.00, '2026-01-15', 'bank_transfer', 'Щомісячна пожертва'),
    (3, 1, 3000.00, '2026-01-20', 'card', 'Разова пожертва'),
    (5, 1, 2000.00, '2026-02-02', 'cash', NULL),
    (4, 1, 8000.00, '2026-02-10', 'bank_transfer', 'Корпоративна допомога'),
    (7, 1, 4000.00, '2026-02-18', 'online', NULL),
    (9, 1, 6000.00, '2026-03-01', 'card', NULL),
    (11, 1, 5000.00, '2026-03-14', 'bank_transfer', NULL),
    (13, 1, 7000.00, '2026-04-05', 'cash', NULL),
    (15, 1, 3000.00, '2026-04-22', 'card', NULL),
    (1, 1, 10000.00, '2026-05-10', 'bank_transfer', 'Щомісячна пожертва'),
    (17, 1, 8000.00, '2026-06-15', 'online', NULL),
    (19, 1, 7000.00, '2026-07-20', 'card', NULL),

    -- Медична допомога (goal 250 000, ~132 500 зібрано)
    (4, 2, 15000.00, '2026-02-12', 'bank_transfer', 'Закупівля обладнання'),
    (2, 2, 10000.00, '2026-02-20', 'cash', NULL),
    (6, 2, 12000.00, '2026-03-03', 'card', NULL),
    (8, 2, 8000.00, '2026-03-19', 'online', NULL),
    (14, 2, 20000.00, '2026-04-01', 'bank_transfer', 'Квартальний внесок'),
    (10, 2, 7000.00, '2026-04-16', 'cash', NULL),
    (12, 2, 9000.00, '2026-05-02', 'card', NULL),
    (16, 2, 11000.00, '2026-05-21', 'bank_transfer', NULL),
    (18, 2, 6000.00, '2026-06-08', 'online', NULL),
    (20, 2, 13500.00, '2026-06-27', 'card', NULL),
    (21, 2, 8000.00, '2026-07-11', 'bank_transfer', NULL),
    (23, 2, 7000.00, '2026-07-30', 'cash', NULL),
    (2, 2, 6000.00, '2026-08-05', 'card', NULL),

    -- Освітні програми (goal 50 000, 100% зібрано, completed)
    (1, 3, 5000.00, '2026-01-25', 'bank_transfer', NULL),
    (3, 3, 8000.00, '2026-02-08', 'card', NULL),
    (5, 3, 6000.00, '2026-02-24', 'online', NULL),
    (7, 3, 4000.00, '2026-03-10', 'cash', NULL),
    (9, 3, 7000.00, '2026-03-27', 'bank_transfer', NULL),
    (11, 3, 5000.00, '2026-04-12', 'card', NULL),
    (13, 3, 6000.00, '2026-04-29', 'online', NULL),
    (15, 3, 4000.00, '2026-05-15', 'cash', NULL),
    (17, 3, 5000.00, '2026-06-01', 'bank_transfer', 'Останній внесок для завершення цілі'),

    -- Зимова підтримка родин (goal 80 000, ~18 400 зібрано)
    (19, 4, 3000.00, '2026-01-08', 'cash', NULL),
    (21, 4, 2000.00, '2026-01-19', 'card', NULL),
    (23, 4, 2400.00, '2026-02-01', 'bank_transfer', NULL),
    (6, 4, 1500.00, '2026-02-14', 'online', NULL),
    (8, 4, 3000.00, '2026-02-28', 'cash', NULL),
    (10, 4, 2500.00, '2026-03-09', 'card', NULL),
    (12, 4, 2000.00, '2026-03-18', 'bank_transfer', NULL),
    (24, 4, 2000.00, '2026-03-25', 'online', NULL);

INSERT INTO beneficiaries (name, contact, project_id, description, assistance_type) VALUES
    ('Дитячий будинок №7', '+380441234567', 1, 'м. Київ, 50 дітей', 'Фінансова підтримка'),
    ('Обласна лікарня', '+380322345678', 2, 'м. Львів, онкологічне відділення', 'Медичне обладнання'),
    ('Студент Андрій Мельник', 'andrii.melnyk@student.edu', 3, 'Студент КПІ, 3 курс', 'Стипендія'),
    ('Родина Гриценків', '+380673456789', 4, 'м. Суми, 4 особи', 'Продуктові набори'),
    ('Школа-інтернат №3', 'school3@edu.ua', 1, 'м. Житомир, 120 учнів', 'Навчальні матеріали'),
    ('Дитячий будинок «Сонечко»', '+380487651234', 1, 'м. Одеса, 35 дітей', 'Фінансова підтримка'),
    ('Міська лікарня №2', '+380577654321', 2, 'м. Харків, кардіологічне відділення', 'Медичне обладнання'),
    ('Студентка Олена Панченко', 'olena.panchenko@student.edu', 3, 'Студентка ЛНУ, 2 курс', 'Стипендія'),
    ('Родина Ковальчуків', '+380663217654', 4, 'м. Полтава, 3 особи', 'Продуктові набори'),
    ('Дитячий будинок №12', '+380567891234', 1, 'м. Дніпро, 42 дитини', 'Фінансова підтримка'),
    ('Онкоцентр', '+380432198765', 2, 'м. Вінниця, дитяче відділення', 'Медичне обладнання'),
    ('Студент Ігор Савчук', 'ihor.savchuk@student.edu', 3, 'Студент НТУУ «КПІ», 4 курс', 'Стипендія'),
    ('Родина Мороз', '+380462345671', 4, 'м. Чернігів, 5 осіб', 'Продуктові набори'),
    ('Реабілітаційний центр «Надія»', '+380362198432', 2, 'м. Рівне, дитяча реабілітація', 'Медичне обладнання'),
    ('Школа-інтернат №1', 'school1.ternopil@edu.ua', 1, 'м. Тернопіль, 95 учнів', 'Навчальні матеріали'),
    ('Студентка Марта Кравець', 'marta.kravets@student.edu', 3, 'Студентка УжНУ, 1 курс', 'Стипендія'),
    ('Родина Бондарів', '+380332198765', 4, 'м. Луцьк, 4 особи', 'Продуктові набори'),
    ('Дитяча лікарня', '+380472198345', 2, 'м. Черкаси, педіатричне відділення', 'Медичне обладнання');

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
