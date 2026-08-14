# Charity Fund

A desktop application for managing a charity fund's donors, donations, projects and beneficiaries — built with **C++17 and Qt6**, backed by **PostgreSQL**. Windows only (MinGW-w64 / MSYS2 toolchain).

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)
![Qt6](https://img.shields.io/badge/Qt-6-41cd52.svg)
![PostgreSQL](https://img.shields.io/badge/database-PostgreSQL-336791.svg)

### Demo / Демо

📹 **[Watch the screen recording / Дивитись відео-демонстрацію](docs/demo.mp4)** — click through to GitHub's built-in video player.

**[English](#english)** · **[Українська](#українська)**

---

## English

### Overview

Charity Fund is a Qt Widgets desktop app for running a small charity fund's back office: donors, donations, projects, beneficiaries, and financial reports with charts. All data lives in PostgreSQL; the app creates and migrates its own schema on first connect, so setup is just "create an empty database" — no manual SQL.

### Features

- Donors — add, edit, search, delete
- Donations — record gifts, link to a donor and (optionally) a project
- Projects — track a fundraising goal vs. amount raised, with progress and status
- Beneficiaries — track who a project's funds went to
- Reports — general/donor/project statistics, bar and pie charts, export to PDF/HTML/text
- Light/dark theme, resizable and collapsible panels
- English/Ukrainian UI

### Tech stack

| Layer | Choice |
|---|---|
| Language | C++17 |
| GUI | Qt6 Widgets + Qt6 Charts |
| Database | PostgreSQL, via Qt6::Sql (`QPSQL` driver) |
| Build | CMake + Ninja, MinGW-w64 (GCC) via MSYS2 |

### Getting started

#### 1. Install MSYS2

Get it from [msys2.org](https://www.msys2.org/) (default install path `C:\msys64` or `%USERPROFILE%\msys64`). This is needed regardless of how you set up PostgreSQL below — it provides the MinGW-w64 compiler toolchain the app is built with.

Open **MSYS2 MSYS** from the Start menu and install the build dependencies:

```bash
pacman -Syu
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts mingw-w64-x86_64-postgresql
```

#### 2. Set up PostgreSQL

Pick one:

**Option A — official installer (recommended).** Download from [postgresql.org/download/windows](https://www.postgresql.org/download/windows/) and run it. This registers PostgreSQL as a real Windows service that starts automatically on every boot — the least error-prone option, and the one to pick if you don't want to think about it again.

**Option B — the PostgreSQL bundled with MSYS2** (already installed in step 1, no extra download). Quicker if you're only doing this once, but it runs as a plain background process, **not** a Windows service — it must be started manually (`pg_ctl -D <datadir> start`) after every reboot, or it won't be running when you launch the app. To make it survive reboots too, register it as a service from an elevated (Administrator) shell:
```bash
pg_ctl register -N postgresql -D /c/pgdata
```

Either way, once the server is running, create the role and an empty database (the app creates all tables itself on first connect):

```bash
psql -U postgres -v ON_ERROR_STOP=1 <<'SQL'
CREATE USER charity_user WITH PASSWORD 'charity_pass';
CREATE DATABASE charity_fund WITH TEMPLATE template0 ENCODING 'UTF8' LC_COLLATE 'C' LC_CTYPE 'C' OWNER charity_user;
SQL
```

`LC_COLLATE`/`LC_CTYPE 'C'` matters: initializing a cluster with a locale inherited from Windows (e.g. `Ukrainian_Ukraine.1251`) is a known cause of backend crashes on `ORDER BY` with non-ASCII text. Don't drop that part even if you skip everything else in this command.

Want a GUI to browse tables and run queries instead of the `psql` command line? Install [pgAdmin](https://www.pgadmin.org/download/pgadmin-4-windows/) — it's a separate, optional download; the app itself doesn't need it.

#### 3. Build

Double-click **`Build.exe`** in the repo root (or run `.\Build.exe` from a terminal). It locates MSYS2, configures the project with CMake+Ninja, builds it, and copies the required Qt DLLs next to the executable.

#### 4. Run

Double-click **`Run.exe`**. On first launch the app creates all tables/indexes/triggers/views in the empty database — no manual schema step needed.

Want demo data (a few donors/projects/donations) instead of starting empty? After step 2, also run:
```bash
psql -h localhost -U charity_user -d charity_fund -f charity-fund/setup_db.sql
```
This script is safe to re-run — it drops and recreates the tables from scratch each time, so only use it before you have real data you care about.

### Configuration

Connection defaults (see `charity-fund/src/database/Database.h`):

| Setting | Default |
|---|---|
| Host | `localhost` |
| Port | `5432` |
| Database | `charity_fund` |
| User | `charity_user` |
| Password | `charity_pass` |

To point the app at a differently-configured database without rebuilding, set any of these environment variables before launching `Run.exe` / `charity_fund.exe`: `CHARITY_DB_HOST`, `CHARITY_DB_PORT`, `CHARITY_DB_NAME`, `CHARITY_DB_USER`, `CHARITY_DB_PASSWORD`. Unset ones fall back to the defaults above.

### Automatic schema migrations

On every successful connection the app runs the bundled `charity-fund/migrations.sql`. It's idempotent (`CREATE TABLE IF NOT EXISTS`, `CREATE OR REPLACE`, etc.) and safe to run repeatedly — it never drops or duplicates anything, only creates what's missing. This means a fresh empty database gets the full schema automatically, and an older database picks up any new tables/triggers added since it was created.

This is separate from `setup_db.sql`, which stays available as an optional script for a full manual reset plus demo data.

### Project structure

```
cursedproject/
├── Run.exe                  # Launches the built app
├── Build.exe                # Builds the app
├── tools/                   # Launcher sources (run_launcher.c, build_launcher.c)
└── charity-fund/
    ├── CMakeLists.txt
    ├── build.ps1
    ├── migrations.sql       # Automatic (idempotent) schema migration
    ├── setup_db.sql         # Optional full reset + demo data
    ├── resources/           # styles.qss / styles_light.qss / .qrc
    └── src/
        ├── main.cpp          # Entry point
        ├── Theme.cpp/.h      # Light/dark theme switching
        ├── Lang.cpp/.h       # English/Ukrainian translation
        ├── models/           # Data models
        ├── database/         # Database (auto-migrate) + Repository
        └── gui/              # Qt6 widgets
```

`Build.exe` / `Run.exe` are thin launchers (sources in `tools/`) that don't require any manual PowerShell/`.bat` juggling. `Build.exe` runs `charity-fund/build.ps1` with the execution policy bypassed for that single run only — it doesn't touch your system's PowerShell policy.

### Troubleshooting

- **Connection error on startup** — make sure PostgreSQL is actually running (see Option A vs. B above — B needs a manual start after every reboot unless you registered it as a service), the `charity_fund` database exists, and `charity_user` can reach it.
- **Migration fails with a permissions error** (`CREATE TABLE`/`CREATE TRIGGER`) — check that `charity_user` is the `OWNER` of the `charity_fund` database, as in the setup command above.
- **`QPSQL driver not loaded` / Qt SQL plugin errors** — confirm `mingw-w64-x86_64-postgresql` is installed (it provides `libpq` for the driver) and that `Build.exe` completed without errors (it copies `qsqlpsql.dll` into `build/sqldrivers/`).
- **Windows blocks the `.exe` as "unknown publisher"** — expected for unsigned builds; click "More info" → "Run anyway".
- **Ukrainian text fails with `character with byte sequence ... has no equivalent in encoding "WIN1251"`** — your PostgreSQL cluster was initialized without `--encoding=UTF8` and inherited a Windows locale encoding. Check with:
  ```bash
  psql -U postgres -c "SELECT datname, pg_encoding_to_char(encoding) FROM pg_database;"
  ```
  If `charity_fund` isn't `UTF8`, recreate it (back up first if you have real data):
  ```bash
  psql -U postgres -v ON_ERROR_STOP=1 <<'SQL'
  DROP DATABASE charity_fund;
  CREATE DATABASE charity_fund WITH TEMPLATE template0 ENCODING 'UTF8' LC_COLLATE 'C' LC_CTYPE 'C' OWNER charity_user;
  SQL
  ```
  The app rebuilds the schema itself on next launch.

### License

[MIT](LICENSE)

---

## Українська

### Огляд

Charity Fund — десктопний застосунок на Qt Widgets для ведення обліку невеликого благодійного фонду: донори, пожертви, проекти, бенефіціари та фінансові звіти з діаграмами. Усі дані зберігаються в PostgreSQL; при першому підключенні застосунок сам створює й мігрує свою схему — все, що потрібно від вас, це порожня база даних, без ручного SQL.

### Можливості

- Донори — додавання, редагування, пошук, видалення
- Пожертви — реєстрація внесків із прив'язкою до донора та (за бажанням) проекту
- Проекти — ціль збору й зібрана сума, з прогресом і статусом
- Бенефіціари — облік того, кому пішли кошти проекту
- Звіти — загальна статистика / по донорах / по проектах, стовпчикові та кругові діаграми, експорт у PDF/HTML/текст
- Світла/темна тема, панелі з можливістю зміни розміру та згортання
- Українська/англійська мова інтерфейсу

### Технології

| Шар | Вибір |
|---|---|
| Мова | C++17 |
| GUI | Qt6 Widgets + Qt6 Charts |
| База даних | PostgreSQL через Qt6::Sql (драйвер `QPSQL`) |
| Збірка | CMake + Ninja, MinGW-w64 (GCC) через MSYS2 |

### Початок роботи

#### 1. Встановіть MSYS2

Завантажте з [msys2.org](https://www.msys2.org/) (типовий шлях `C:\msys64` або `%USERPROFILE%\msys64`). Це потрібно незалежно від того, який варіант PostgreSQL ви оберете нижче — MSYS2 дає компілятор MinGW-w64, яким збирається застосунок.

Відкрийте **MSYS2 MSYS** з меню Пуск і встановіть залежності для збірки:

```bash
pacman -Syu
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts mingw-w64-x86_64-postgresql
```

#### 2. Налаштуйте PostgreSQL

Оберіть один із варіантів:

**Варіант A — офіційний інсталятор (рекомендовано).** Завантажте з [postgresql.org/download/windows](https://www.postgresql.org/download/windows/) і встановіть. Він реєструє PostgreSQL як справжню службу Windows, що стартує автоматично при кожному завантаженні системи — найменш проблемний варіант, і саме той, який варто обрати, якщо не хочете думати про це знову.

**Варіант B — PostgreSQL, що йде в комплекті з MSYS2** (уже встановлений на кроці 1, без додаткового завантаження). Швидше, якщо робите це один раз, але працює як звичайний фоновий процес, а **не** служба Windows — після кожного перезавантаження його потрібно запускати вручну (`pg_ctl -D <каталог_даних> start`), інакше на момент запуску застосунку його просто не буде. Щоб він теж переживав перезавантаження, зареєструйте його як службу з термінала від імені адміністратора:
```bash
pg_ctl register -N postgresql -D /c/pgdata
```

У будь-якому разі, коли сервер запущено, створіть роль і порожню базу даних (усі таблиці застосунок створить сам при першому підключенні):

```bash
psql -U postgres -v ON_ERROR_STOP=1 <<'SQL'
CREATE USER charity_user WITH PASSWORD 'charity_pass';
CREATE DATABASE charity_fund WITH TEMPLATE template0 ENCODING 'UTF8' LC_COLLATE 'C' LC_CTYPE 'C' OWNER charity_user;
SQL
```

`LC_COLLATE`/`LC_CTYPE 'C'` тут важливі: якщо кластер ініціалізовано з локаллю, успадкованою від Windows (наприклад, `Ukrainian_Ukraine.1251`), це відома причина крашів бекенда на `ORDER BY` з не-ASCII текстом. Не пропускайте цю частину команди, навіть якщо решту спростите.

Хочете графічний інтерфейс для перегляду таблиць і запитів замість командного рядка `psql`? Встановіть [pgAdmin](https://www.pgadmin.org/download/pgadmin-4-windows/) — це окреме опційне завантаження, самому застосунку воно не потрібне.

#### 3. Зберіть застосунок

Двічі клацніть **`Build.exe`** в корені репозиторію (або запустіть `.\Build.exe` в терміналі). Він сам знаходить MSYS2, конфігурує проект через CMake+Ninja, збирає його та копіює потрібні Qt DLL поруч із виконуваним файлом.

#### 4. Запустіть

Двічі клацніть **`Run.exe`**. При першому запуску застосунок сам створить усі таблиці/індекси/тригери/в'юхи в порожній базі — жодних ручних SQL-команд для схеми не потрібно.

Хочете одразу побачити застосунок із демо-даними (кілька донорів/проектів/пожертв) замість порожньої бази? Після кроку 2 додатково виконайте:
```bash
psql -h localhost -U charity_user -d charity_fund -f charity-fund/setup_db.sql
```
Цей скрипт можна запускати повторно — він щоразу перестворює таблиці з нуля, тож використовуйте його лише до появи реальних даних, які шкода втратити.

### Налаштування підключення

Параметри за замовчуванням (див. `charity-fund/src/database/Database.h`):

| Параметр | Значення за замовчуванням |
|---|---|
| Host | `localhost` |
| Port | `5432` |
| База даних | `charity_fund` |
| Користувач | `charity_user` |
| Пароль | `charity_pass` |

Щоб підключити застосунок до інакше налаштованої бази без перезбірки, задайте перед запуском `Run.exe` / `charity_fund.exe` будь-які зі змінних середовища: `CHARITY_DB_HOST`, `CHARITY_DB_PORT`, `CHARITY_DB_NAME`, `CHARITY_DB_USER`, `CHARITY_DB_PASSWORD`. Незадані — використовують значення за замовчуванням вище.

### Автоматичні міграції схеми

При кожному успішному підключенні застосунок виконує вбудований `charity-fund/migrations.sql`. Він ідемпотентний (`CREATE TABLE IF NOT EXISTS`, `CREATE OR REPLACE` тощо) і безпечний для повторного запуску — нічого не видаляє й не дублює, лише створює те, чого ще немає. Завдяки цьому нова порожня база отримує повну схему автоматично, а старіша база підхоплює будь-які нові таблиці/тригери, додані вже після її створення.

Це окремо від `setup_db.sql`, який лишається опційним скриптом для повного ручного ресету з демо-даними.

### Структура проекту

```
cursedproject/
├── Run.exe                  # Запуск зібраного застосунку
├── Build.exe                # Збірка застосунку
├── tools/                   # Джерела лаунчерів (run_launcher.c, build_launcher.c)
└── charity-fund/
    ├── CMakeLists.txt
    ├── build.ps1
    ├── migrations.sql       # Автоматична (ідемпотентна) міграція схеми
    ├── setup_db.sql         # Опційний повний ресет + демо-дані
    ├── resources/           # styles.qss / styles_light.qss / .qrc
    └── src/
        ├── main.cpp          # Точка входу
        ├── Theme.cpp/.h      # Перемикання світлої/темної теми
        ├── Lang.cpp/.h       # Переклад укр/англ
        ├── models/           # Моделі даних
        ├── database/         # Database (з авто-міграцією) + Repository
        └── gui/              # Qt6 інтерфейс
```

`Build.exe` / `Run.exe` — прості лаунчери (джерела в `tools/`), які не вимагають ручної роботи з PowerShell/`.bat`. `Build.exe` запускає `charity-fund/build.ps1` з обходом execution policy лише для цього одного запуску — системні налаштування PowerShell не змінюються.

### Типові проблеми

- **Помилка підключення при старті** — перевірте, що PostgreSQL справді запущений (див. варіанти A/B вище — варіант B потребує ручного запуску після кожного перезавантаження, якщо ви не зареєстрували його як службу), база `charity_fund` існує, і `charity_user` має до неї доступ.
- **Міграція падає з помилкою прав** (`CREATE TABLE`/`CREATE TRIGGER`) — переконайтесь, що `charity_user` є власником (`OWNER`) бази `charity_fund`, як у команді налаштування вище.
- **Помилки `QPSQL driver not loaded` / Qt SQL plugin** — перевірте, що встановлено пакет `mingw-w64-x86_64-postgresql` (додає `libpq` для драйвера) і що `Build.exe` завершився без помилок (він копіює `qsqlpsql.dll` у `build/sqldrivers/`).
- **Windows блокує `.exe` як "невідомого видавця"** — очікувано для непідписаних збірок; натисніть "Докладніше" → "Виконати все одно".
- **Українські символи не відображаються**, помилка на кшталт `character with byte sequence ... has no equivalent in encoding "WIN1251"` — кластер PostgreSQL ініціалізовано без `--encoding=UTF8` і він успадкував кодування з локалі Windows. Перевірити можна так:
  ```bash
  psql -U postgres -c "SELECT datname, pg_encoding_to_char(encoding) FROM pg_database;"
  ```
  Якщо `charity_fund` не в `UTF8`, пересоздайте базу (за потреби зробіть дамп заздалегідь):
  ```bash
  psql -U postgres -v ON_ERROR_STOP=1 <<'SQL'
  DROP DATABASE charity_fund;
  CREATE DATABASE charity_fund WITH TEMPLATE template0 ENCODING 'UTF8' LC_COLLATE 'C' LC_CTYPE 'C' OWNER charity_user;
  SQL
  ```
  Схему застосунок відтворить сам при наступному запуску.

### Ліцензія

[MIT](LICENSE)
