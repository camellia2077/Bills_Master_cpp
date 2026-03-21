# bills_android

Android MVP app for importing one bundled `testdata/bills/2025/2025-01.txt`
sample into a private SQLite database and querying fixed year/month summaries
with Compose.

## Commands

- `./gradlew :apps:bills_android:tasks`
- `./gradlew :apps:bills_android:assembleDebug`
- `python tools/flows/distribute_configs.py --targets android`

## Layout

- Gradle outputs: `apps/bills_android/build`
- Native staging: `apps/bills_android/.cxx`
- Bundled input: root `testdata/bills/2025/2025-01.txt`
- Bundled config source: root `config/*.toml`
- Bundled Android config payload: `dist/config/android/*.toml`

## Docs

- `apps/bills_android/AGENTS.md`
- `docs/modules/bills_android/README.md`
- `docs/modules/bills_android/architecture.md`
- `docs/modules/bills_android/module_map.md`
- `docs/modules/bills_android/change_guide.md`
- `docs/modules/bills_android/dist_and_test.md`
