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
