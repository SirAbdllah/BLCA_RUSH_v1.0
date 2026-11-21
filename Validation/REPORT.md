# Validation / التحقق

Date: 2026-11-21 · Game/Tools: 1.9.0.37

## Completed

- Game and WorkbenchGame compiled with only the standalone project and vanilla data.
- 64 Workbench checks passed: Arland world, native managers, UI/map textures, M-COM/squad/radio prefabs, four sectors, and all 24 objective/deployment positions above dry terrain.
- 22 dedicated-server checks passed: fresh state, deployment points, ticket reservations, defuse cancellation, overtime, timeout victory, detonation, all sector transitions, and native attacker victory.
- Native round reload was observed with fresh initialization 5 times; ticket and round state reset correctly.
- Two real local diagnostic clients joined an isolated dedicated server. Initial deployment, joining mid-round, squad deployment, rejecting an out-of-range objective RPC, and an accepted arm hold through the owned-controller RPC all passed.
- Resource audit passed: 13 packaged references and 33 vanilla references; no third-party add-on dependency.
- JSON examples, local documentation links, and Python syntax checked.

## Method and limits

Network tests used a private fixture to start a round without an opposing participant and allow two same-team clients for the squad test. That fixture, its settings, profiles, log identities, and paths are excluded from the release. Normal release rules still require two players on opposing teams.

No PlayStation/Xbox hardware test or populated public-server load test is claimed. Native 1.9 data may emit Rally-DLC listing diagnostics, and Workbench can report native resource leaks while closing. During Arland world teardown, two stock resupply stations reported that their entity-catalog manager was already removed; the next rounds still initialized and completed successfully. No add-on script compilation errors remain in these checks.

Full machine-readable results and sanitized log digests: `results.json`.

## العربية

نجح تجميع المشروع مع اللعبة الأساسية وحدها، ونجحت 64 عملية فحص في Workbench و22 عملية فحص للجولة على سيرفر فعلي. دخل عميلان محليان، ونجح النزول الأساسي والدخول أثناء الجولة ونزول السكواد وتفعيل البومب عبر RPC، مع رفض طلب هدف بعيد عن اللاعب.

فحص الاعتماديات أكد أن المراجع تتجه إلى موارد الحزمة أو اللعبة الأساسية فقط. اختبارات الشبكة استخدمت تهيئة خاصة لبدء الجولة واختبار السكواد بنفس الفريق؛ هذه التهيئة ليست في النسخة المنشورة. لم يُجر اختبار بأجهزة PlayStation/Xbox أو سيرفر عام ممتلئ.
