# Map integration / تركيب الطور على خريطة

## العربية

سيناريو Arland مثال كامل مبني على خريطة اللعبة الأساسية. لإنشاء سيناريو جديد:

1. انسخ ملف العالم وطبقة `Rush.layer` إلى اسم جديد، وأنشئ لهما الموارد/المعرّفات الجديدة من Workbench. لا تكرر GUID لعالم أو سيناريو مختلف.
2. اجعل `Parent` خريطة الأرض المطلوبة. الخريطة الخارجية تحتاج اعتمادًا عليها في **إضافة التكامل**؛ نواة BLCA RUSH تظل مستقلة.
3. احتفظ بقيم مود واحد `BLCA_GameMode.et`، ومدير فصائل US/USSR، ومدير لوداوت أصلي، ومدير AI والراديو والإدخال والدردشة اللازمين كما في المثال.
4. لكل سكتر أنشئ `GenericEntity` بمكوّن `BLCA_RushSectorComponent`. رقّم `Index` من 0 بالتسلسل، دون تكرار أو فجوات.
5. ضع هدفين من `BLCA_MCOM.et`، بمكوّن `Sector` مطابق للسكتر، و`Letter=0` لـA و`Letter=1` لـB.
6. ضع نقطتي نزول أصليتين US وUSSR. اربط أسماء الكيانات في `AttackSpawn` و`DefendSpawn` و`ObjectiveA` و`ObjectiveB`. الأسماء يجب أن تكون فريدة في العالم.
7. يمكن وضع نقطتي دفاع إضافيتين وربطهما بـ`DefendSpawnA/B`. حساب مكان النزول النهائي يبحث قرب الهدف على مسافة تقارب 100 متر، ولا يضع الشخصية فوق البومب نفسها.
8. غيّر `Title` و`BoundaryRadius`، وأبقِ الأهداف والنقاط فوق أرض جافة وفي أماكن خالية من العوائق. ممر الاقتراب بين المركز ونقطتي النزول يدخل ضمن حدود السكتر.
9. حدّث معرّف العالم في رأس السيناريو `Missions/*.conf`، والمؤلف والاسم والصور.
10. الخريطة الكبيرة تأتي من العالم نفسه. صور المني ماب في `BLCA_RushHUDController.c`: `MAP_TEXTURE` و`MAP_DETAIL`؛ استبدلهما بصور الخريطة المناسبة، وبأبعاد/إزاحة تطابق `SCR_MapEntity`.

استخدم `BLCA_RushSurvey.c` مثالًا لفحص مراجع السيناريو وارتفاعات الكيانات؛ حدّث مسار العالم وعدد السكترات والأسماء والموارد وفق سيناريوك. لا تحتاج تغيير سكربت الجولة أو الرسبون لإعادة ترتيب الأهداف، بل عدّل بيانات طبقة العالم.

## English

The Arland scenario is a complete vanilla-terrain example.

1. Duplicate its world and `Rush.layer` under fresh names and assign new resource GUIDs in Workbench.
2. Set `Parent` to the desired terrain. External terrain dependencies belong in a separate integration add-on.
3. Keep one `BLCA_GameMode.et`, native US/USSR factions and roles, and the supporting managers from the example.
4. Create a sector entity with `BLCA_RushSectorComponent`; indices must be unique, contiguous, and start at zero.
5. Place two `BLCA_MCOM.et` terminals per sector, with matching `Sector` and `Letter` 0/1.
6. Place native US/USSR spawn points and link unique entity names through `ObjectiveA/B` and `AttackSpawn/DefendSpawn`.
7. Optional `DefendSpawnA/B` points enable objective deployment. Final position search targets a safe position about 100 metres from the terminal.
8. Set `Title` and `BoundaryRadius`. Place objectives and points on dry ground clear of obstacles. Spawn approach corridors are included in the combat boundary.
9. Create/update the mission header with the new world, identity, author, and artwork.
10. The full map comes from the world. Update minimap `MAP_TEXTURE` and `MAP_DETAIL` in `BLCA_RushHUDController.c` for a different terrain, matching the native map's dimensions and offset.

Adapt `BLCA_RushSurvey.c` for your world, sector count, names, and resources. Objective placement is world data; changing coordinates does not require rewriting authoritative round or respawn logic.
