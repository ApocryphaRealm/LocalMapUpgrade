# -*- coding: utf-8 -*-
"""gen-translations.py - builds the eleven LocalMapUpgrade_<language>.txt files.

The English key list is extracted from the PATCHED source/UI.cpp by regex on
strings::TR("KEY", "text") so it can never drift from the code. The other ten languages are
this project's own translations of that list, held below as parallel dictionaries.

Writes REPO/dist/Interface/Translations/LocalMapUpgrade_<language>.txt for english + the owner's
ten languages (UTF-16LE with a BOM, one "$key<TAB>text" per line, literal "\\n" for an embedded
line break, CRLF records - the SKSE/SkyUI shape AMF's own Strings.cpp reads).

Run: `python tools/gen-translations.py` from the repo root or anywhere (paths are relative to
this script's grandparent directory).
"""
import io
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANGS = ["english", "japanese", "korean", "chinese", "russian", "german", "french", "spanish", "italian", "polish", "czech"]

TR_RE = re.compile(r'strings::TR\(\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)')


def unescape(s):
    return s.encode("latin-1", "backslashreplace").decode("unicode_escape") if "\\" in s else s


KEY_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelKeys\[\]\s*=\s*\{([^}]*)\};', re.S)
NAME_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelNames\[\]\s*=\s*\{([^}]*)\};', re.S)
STR_LIT_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def read_keys():
    path = os.path.join(REPO, "source", "UI.cpp")
    src = io.open(path, "r", encoding="utf-8").read()
    keys = {}
    order = []
    for m in TR_RE.finditer(src):
        key, text = unescape(m.group(1)), unescape(m.group(2))
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    # The log-level Combo's option labels are looked up by a parallel key array
    # (kLogLevelKeys[i] -> kLogLevelNames[i]) rather than a literal strings::TR(...) call, since
    # the option text is rebuilt into a std::vector per frame. Pair the two arrays positionally.
    km = KEY_ARRAY_RE.search(src)
    nm = NAME_ARRAY_RE.search(src)
    if not km or not nm:
        raise RuntimeError("could not find kLogLevelKeys/kLogLevelNames arrays in source/UI.cpp")
    array_keys = [unescape(s) for s in STR_LIT_RE.findall(km.group(1))]
    array_names = [unescape(s) for s in STR_LIT_RE.findall(nm.group(1))]
    if len(array_keys) != len(array_names):
        raise RuntimeError(f"kLogLevelKeys ({len(array_keys)}) and kLogLevelNames ({len(array_names)}) length mismatch")
    for key, text in zip(array_keys, array_names):
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    return keys, order


# ------------------------------------------------------------------------------------------------
# Translations for every key found in source/UI.cpp. Plain, literal renderings of the UI text;
# every printf specifier is kept exactly; product names (Skyrim, Apocrypha Menu Framework, Local
# Map Upgrade, Untarnished UI, Dragon's Eye Minimap) stay untranslated; file names (the INI) stay
# untranslated inside the translated sentence; Skyrim's own terms (local map, marker, quest,
# hostile, actor, door, ally) use each language's in-game vocabulary.
# ------------------------------------------------------------------------------------------------
TRANSLATIONS = {
    "LMU_HelpMark": {
        "japanese": "(?)", "korean": "(?)", "chinese": "(?)", "russian": "(?)", "german": "(?)",
        "french": "(?)", "spanish": "(?)", "italian": "(?)", "polish": "(?)", "czech": "(?)",
    },
    "LMU_SliderNudge": {
        "japanese": "<-->", "korean": "<-->", "chinese": "<-->", "russian": "<-->", "german": "<-->",
        "french": "<-->", "spanish": "<-->", "italian": "<-->", "polish": "<-->", "czech": "<-->",
    },
    "LMU_Title": {
        "japanese": "ローカルマップ", "korean": "로컬 맵", "chinese": "本地地图", "russian": "Локальная карта",
        "german": "Lokale Karte", "french": "Carte locale", "spanish": "Mapa local",
        "italian": "Mappa locale", "polish": "Mapa lokalna", "czech": "Místní mapa",
    },
    "LMU_Color": {
        "japanese": "カラー", "korean": "색상", "chinese": "彩色", "russian": "Цвет", "german": "Farbe",
        "french": "Couleur", "spanish": "Color", "italian": "Colore", "polish": "Kolor", "czech": "Barva",
    },
    "LMU_HelpColor": {
        "japanese": "ローカル(ダンジョン/屋内)マップをバニラの黒白ではなくカラーで表示します。",
        "korean": "로컬(던전/실내) 맵을 바닐라의 흑백 대신 컬러로 표시합니다.",
        "chinese": "以彩色而非原版的黑白显示本地(地下城/室内)地图。",
        "russian": "Отображает локальную (подземелье/интерьер) карту в цвете вместо чёрно-белой карты по умолчанию.",
        "german": "Zeigt die lokale Karte (Dungeon/Innenbereich) in Farbe statt im Standard-Schwarzweiß an.",
        "french": "Affiche la carte locale (donjon/intérieur) en couleur au lieu du noir et blanc d'origine.",
        "spanish": "Muestra el mapa local (mazmorra/interior) en color en lugar del blanco y negro original.",
        "italian": "Mostra la mappa locale (dungeon/interno) a colori invece del bianco e nero originale.",
        "polish": "Wyświetla mapę lokalną (podziemia/wnętrze) w kolorze zamiast domyślnej czarno-białej.",
        "czech": "Zobrazí místní mapu (dungeon/interiér) barevně místo výchozí černobílé.",
    },
    "LMU_FogOfWar": {
        "japanese": "戦場の霧", "korean": "전장의 안개", "chinese": "战争之雾", "russian": "Туман войны",
        "german": "Nebel des Krieges", "french": "Brouillard de guerre", "spanish": "Niebla de guerra",
        "italian": "Nebbia di guerra", "polish": "Mgła wojny", "czech": "Válečná mlha",
    },
    "LMU_HelpFogOfWar": {
        "japanese": "ローカルマップの未探索部分を隠したままにするかどうか。無効にするとマップ全体が表示されます。",
        "korean": "로컬 맵의 미탐색 부분을 계속 숨길지 여부입니다. 비활성화하면 맵 전체가 드러납니다.",
        "chinese": "本地地图未探索的部分是否保持隐藏。关闭后将显示整张地图。",
        "russian": "Остаются ли неразведанные части локальной карты скрытыми. Отключение раскрывает всю карту.",
        "german": "Ob unerforschte Teile der lokalen Karte verborgen bleiben. Deaktivieren zeigt die gesamte Karte.",
        "french": "Indique si les parties inexplorées de la carte locale restent masquées. La désactivation révèle toute la carte.",
        "spanish": "Si las partes sin explorar del mapa local permanecen ocultas. Al desactivarlo se revela todo el mapa.",
        "italian": "Se le parti inesplorate della mappa locale restano nascoste. Disattivandolo si rivela l'intera mappa.",
        "polish": "Czy nieodkryte części mapy lokalnej pozostają skryte. Wyłączenie odkrywa całą mapę.",
        "czech": "Zda neprozkoumané části místní mapy zůstanou skryté. Vypnutím se odhalí celá mapa.",
    },
    "LMU_KeyboardPanSpeed": {
        "japanese": "キーボードでのパン速度", "korean": "키보드 이동 속도", "chinese": "键盘平移速度",
        "russian": "Скорость перемещения клавиатурой", "german": "Kartenschwenk-Geschwindigkeit (Tastatur)",
        "french": "Vitesse de défilement au clavier", "spanish": "Velocidad de desplazamiento con teclado",
        "italian": "Velocità di scorrimento da tastiera", "polish": "Prędkość przesuwania klawiaturą",
        "czech": "Rychlost posunu klávesnicí",
    },
    "LMU_HelpKeyboardPanSpeed": {
        "japanese": "キーボードでローカルマップをパンする速度。",
        "korean": "키보드로 로컬 맵을 이동시킬 때의 속도입니다.",
        "chinese": "使用键盘平移本地地图时的速度。",
        "russian": "Насколько быстро локальная карта перемещается при управлении клавиатурой.",
        "german": "Wie schnell die lokale Karte beim Schwenken mit der Tastatur bewegt wird.",
        "french": "À quelle vitesse la carte locale défile lorsqu'on la déplace au clavier.",
        "spanish": "Con qué rapidez se desplaza el mapa local al moverlo con el teclado.",
        "italian": "Quanto velocemente scorre la mappa locale quando viene spostata con la tastiera.",
        "polish": "Jak szybko mapa lokalna przesuwa się podczas przesuwania klawiaturą.",
        "czech": "Jak rychle se místní mapa posouvá při posunu klávesnicí.",
    },
    "LMU_ActorMarkers": {
        "japanese": "アクターマーカー", "korean": "액터 마커", "chinese": "角色标记", "russian": "Маркеры персонажей",
        "german": "Akteur-Markierungen", "french": "Marqueurs d'acteurs", "spanish": "Marcadores de actores",
        "italian": "Indicatori degli attori", "polish": "Znaczniki postaci", "czech": "Značky postav",
    },
    "LMU_ShowEnemyActors": {
        "japanese": "敵アクターを表示", "korean": "적 액터 표시", "chinese": "显示敌方角色",
        "russian": "Показывать врагов", "german": "Feindliche Akteure anzeigen", "french": "Afficher les ennemis",
        "spanish": "Mostrar actores enemigos", "italian": "Mostra attori nemici", "polish": "Pokaż wrogie postacie",
        "czech": "Zobrazit nepřátelské postavy",
    },
    "LMU_ShowHostileActors": {
        "japanese": "敵対的アクターを表示", "korean": "적대적 액터 표시", "chinese": "显示敌意角色",
        "russian": "Показывать враждебных", "german": "Feindselige Akteure anzeigen", "french": "Afficher les hostiles",
        "spanish": "Mostrar actores hostiles", "italian": "Mostra attori ostili", "polish": "Pokaż wrogo nastawione postacie",
        "czech": "Zobrazit nepřátelsky naladěné postavy",
    },
    "LMU_ShowGuardActors": {
        "japanese": "衛兵アクターを表示", "korean": "경비병 액터 표시", "chinese": "显示卫兵角色",
        "russian": "Показывать стражников", "german": "Wachen anzeigen", "french": "Afficher les gardes",
        "spanish": "Mostrar actores guardias", "italian": "Mostra attori guardie", "polish": "Pokaż postacie strażników",
        "czech": "Zobrazit postavy strážných",
    },
    "LMU_ShowDeadActors": {
        "japanese": "死亡アクターを表示", "korean": "사망한 액터 표시", "chinese": "显示死亡角色",
        "russian": "Показывать мёртвых", "german": "Tote Akteure anzeigen", "french": "Afficher les morts",
        "spanish": "Mostrar actores muertos", "italian": "Mostra attori morti", "polish": "Pokaż zabite postacie",
        "czech": "Zobrazit mrtvé postavy",
    },
    "LMU_ShowTeammateActors": {
        "japanese": "仲間アクターを表示", "korean": "동료 액터 표시", "chinese": "显示队友角色",
        "russian": "Показывать союзников", "german": "Verbündete Akteure anzeigen", "french": "Afficher les alliés",
        "spanish": "Mostrar actores aliados", "italian": "Mostra attori alleati", "polish": "Pokaż postacie sojusznicze",
        "czech": "Zobrazit spřátelené postavy",
    },
    "LMU_ShowNeutralActors": {
        "japanese": "中立アクターを表示", "korean": "중립 액터 표시", "chinese": "显示中立角色",
        "russian": "Показывать нейтральных", "german": "Neutrale Akteure anzeigen", "french": "Afficher les neutres",
        "spanish": "Mostrar actores neutrales", "italian": "Mostra attori neutrali", "polish": "Pokaż neutralne postacie",
        "czech": "Zobrazit neutrální postavy",
    },
    "LMU_ImmersiveMode": {
        "japanese": "イモーシブモード", "korean": "이머시브 모드", "chinese": "沉浸模式", "russian": "Иммерсивный режим",
        "german": "Immersiver Modus", "french": "Mode immersif", "spanish": "Modo inmersivo",
        "italian": "Modalità immersiva", "polish": "Tryb immersyjny", "czech": "Imerzivní režim",
    },
    "LMU_HelpImmersiveMode": {
        "japanese": "生命/死を感知する効果が有効な間だけローカルマップにアクターマーカーを表示します(常時ではなく)。既定ではオフ - 感知効果に紐付けたい場合はオンにしてください。",
        "korean": "생명체/사망체 감지 효과가 활성화되어 있을 때만 로컬 맵에 액터 마커를 표시합니다(항상 표시가 아님). 기본값은 꺼짐 - 감지 효과에 연동하려면 켜세요.",
        "chinese": "仅在生命/死亡感知效果生效时,才在本地地图上显示角色标记(而非始终显示)。默认关闭 - 如果你希望标记依赖感知效果,请将其打开。",
        "russian": "Показывает маркеры персонажей на локальной карте только при активном эффекте обнаружения живых/нежити, а не всегда. По умолчанию выключено - включите, если хотите привязать маркеры к эффекту обнаружения.",
        "german": "Zeigt Akteur-Markierungen auf der lokalen Karte nur an, während ein Detect-Life/Dead-Effekt aktiv ist, statt immer. Standardmäßig aus - aktiviere es, wenn die Markierungen an einen Detect-Effekt gebunden sein sollen.",
        "french": "Affiche les marqueurs d'acteurs sur la carte locale uniquement pendant qu'un effet de détection des vivants/morts est actif, au lieu de toujours. Désactivé par défaut - activez-le si vous voulez que les marqueurs dépendent d'un effet de détection.",
        "spanish": "Muestra los marcadores de actores en el mapa local solo mientras un efecto de detección de vivos/muertos esté activo, en lugar de siempre. Desactivado por defecto - actívalo si quieres que los marcadores dependan de un efecto de detección.",
        "italian": "Mostra gli indicatori degli attori sulla mappa locale solo mentre è attivo un effetto di rilevamento vivi/morti, invece che sempre. Disattivato per impostazione predefinita - attivalo se vuoi che gli indicatori dipendano da un effetto di rilevamento.",
        "polish": "Pokazuje znaczniki postaci na mapie lokalnej tylko wtedy, gdy aktywny jest efekt wykrywania żywych/nieumarłych, a nie zawsze. Domyślnie wyłączone - włącz, jeśli chcesz, aby znaczniki zależały od efektu wykrywania.",
        "czech": "Zobrazuje značky postav na místní mapě pouze tehdy, když je aktivní efekt zjištění živých/nemrtvých, ne vždy. Výchozí stav: vypnuto - zapněte, pokud chcete, aby značky závisely na zjišťovacím efektu.",
    },
    "LMU_MapBorder": {
        "japanese": "マップの枠", "korean": "맵 테두리", "chinese": "地图边框", "russian": "Рамка карты",
        "german": "Kartenrahmen", "french": "Bordure de carte", "spanish": "Borde del mapa",
        "italian": "Bordo della mappa", "polish": "Ramka mapy", "czech": "Rámeček mapy",
    },
    "LMU_HelpMapBorder": {
        "japanese": "ローカルマップの周りに枠を描画します。既定はオフ:ゲーム自体が独自の枠を描き、多くのUIリプレイサーも枠を描くため、有効にすると誰かの枠に重ねて二重に表示されることがあります。リプレイサーがバニラの枠を取り除いた場合、またはこちらの枠を好む場合に有効にしてください。マップを開き直さなくてもすぐに適用されます。",
        "korean": "로컬 맵 주위에 테두리를 그립니다. 기본값은 꺼짐: 게임 자체가 자체 테두리를 그리고, 대부분의 UI 교체 모드도 테두리를 그리므로, 그렇지 않으면 다른 모드의 테두리 위에 두 번째 테두리가 겹칠 수 있습니다. 교체 모드가 바닐라 테두리를 제거했거나 이 테두리를 선호한다면 켜세요. 맵을 다시 열지 않아도 즉시 적용됩니다.",
        "chinese": "在本地地图周围绘制边框。默认关闭:游戏本身会绘制自己的边框,大多数 UI 替换模组也会绘制边框,因此启用后可能会在别人的边框上再叠加一层。如果你的替换模组移除了原版边框,或者你更喜欢我们的边框,请打开此选项。此设置会实时生效,无需重新打开地图。",
        "russian": "Рисует рамку вокруг локальной карты. По умолчанию выключено: сама игра рисует свою рамку, и большинство UI-replacer'ов делают то же самое, поэтому включение может наложить вторую рамку на чужую. Включите, если ваш replacer убрал ванильную рамку или вы предпочитаете эту. Применяется сразу же - карту заново открывать не нужно.",
        "german": "Zeichnet einen Rahmen um die lokale Karte. Standardmäßig aus: Das Spiel zeichnet bereits seinen eigenen Rahmen, und die meisten UI-Replacer tun dasselbe, sodass dies sonst einen zweiten Rahmen über den eines anderen legen würde. Aktiviere es, wenn dein Replacer den Standardrahmen entfernt hat oder du unseren bevorzugst. Wirkt sofort - die Karte muss nicht neu geöffnet werden.",
        "french": "Dessine une bordure autour de la carte locale. Désactivé par défaut : le jeu dessine déjà sa propre bordure, et la plupart des remplaçants d'interface aussi, donc cela superposerait sinon une seconde bordure à celle d'un autre. Activez-le si votre remplaçant a retiré la bordure d'origine, ou si vous préférez la nôtre. S'applique immédiatement - pas besoin de rouvrir la carte.",
        "spanish": "Dibuja un borde alrededor del mapa local. Desactivado por defecto: el propio juego dibuja su propio borde, y la mayoría de los sustitutos de interfaz también, así que de otro modo esto apilaría un segundo borde sobre el de otro. Actívalo si tu sustituto eliminó el borde original, o si prefieres el nuestro. Se aplica de inmediato - no es necesario volver a abrir el mapa.",
        "italian": "Disegna un bordo attorno alla mappa locale. Disattivato per impostazione predefinita: il gioco stesso disegna il proprio bordo, e la maggior parte dei sostituti dell'interfaccia fa lo stesso, quindi altrimenti si sovrapporrebbe un secondo bordo a quello di qualcun altro. Attivalo se il tuo sostituto ha rimosso il bordo originale, o se preferisci il nostro. Si applica immediatamente - non serve riaprire la mappa.",
        "polish": "Rysuje ramkę wokół mapy lokalnej. Domyślnie wyłączone: sama gra rysuje własną ramkę, a większość zamienników interfejsu robi to samo, więc w przeciwnym razie nałożyłoby to drugą ramkę na czyjąś. Włącz, jeśli twój zamiennik usunął domyślną ramkę lub wolisz naszą. Działa od razu - nie trzeba ponownie otwierać mapy.",
        "czech": "Vykreslí rámeček kolem místní mapy. Výchozí stav: vypnuto - samotná hra vykresluje svůj vlastní rámeček a většina náhrad rozhraní dělá totéž, takže by se jinak přidal druhý rámeček na cizí. Zapněte, pokud vaše náhrada odstranila výchozí rámeček, nebo pokud dáváte přednost tomuto. Použije se okamžitě - mapu není třeba znovu otevírat.",
    },
    "LMU_BorderStyle": {
        "japanese": "枠のスタイル", "korean": "테두리 스타일", "chinese": "边框样式", "russian": "Стиль рамки",
        "german": "Rahmenstil", "french": "Style de bordure", "spanish": "Estilo de borde",
        "italian": "Stile del bordo", "polish": "Styl ramki", "czech": "Styl rámečku",
    },
    "LMU_HelpBorderStyle": {
        "japanese": "Skyrim:結び目模様の枠 - メニューフレームワークのSkyrimテーマが使う同じノルドの装飾をマップの周りに描きます。既定。Untarnished:Untarnished UIのオフホワイトによる単純な一本線。",
        "korean": "Skyrim: 매듭 무늬 테두리 - 메뉴 프레임워크의 Skyrim 테마가 사용하는 것과 같은 노르드 문양을 맵 둘레에 그립니다. 기본값입니다. Untarnished: Untarnished UI의 오프화이트 색상으로 된 단순한 한 줄 선.",
        "chinese": "Skyrim:结绳纹边框 - 与菜单框架的 Skyrim 主题所用相同的诺德风格纹饰,绘制在地图四周。默认样式。Untarnished:Untarnished UI 米白色的简单单线边框。",
        "russian": "Skyrim: рамка с плетёным узором - тот же нордский орнамент, что использует тема Skyrim в фреймворке меню, нарисованный вокруг карты. Значение по умолчанию. Untarnished: простая одинарная линия в цвете off-white из Untarnished UI.",
        "german": "Skyrim: der Knotenrahmen - dieselbe nordische Verzierung, die das Skyrim-Theme des Menü-Frameworks verwendet, rund um die Karte gezeichnet. Der Standard. Untarnished: eine einfache einzelne Linie im Off-White von Untarnished UI.",
        "french": "Skyrim : la bordure à motifs entrelacés - le même ornement nordique que celui utilisé par le thème Skyrim du framework de menu, dessiné autour de la carte. La valeur par défaut. Untarnished : une simple ligne unique dans le blanc cassé d'Untarnished UI.",
        "spanish": "Skyrim: el marco de nudos - el mismo ornamento nórdico que usa el tema Skyrim del framework de menús, dibujado alrededor del mapa. El valor predeterminado. Untarnished: una simple línea única en el blanco roto de Untarnished UI.",
        "italian": "Skyrim: la cornice a nodi - lo stesso ornamento nordico usato dal tema Skyrim del framework dei menu, disegnato attorno alla mappa. Il valore predefinito. Untarnished: una semplice linea unica nel bianco sporco di Untarnished UI.",
        "polish": "Skyrim: ramka z plecionym wzorem - ten sam nordycki ornament, którego używa temat Skyrim frameworku menu, narysowany wokół mapy. Wartość domyślna. Untarnished: prosta pojedyncza linia w kolorze złamanej bieli z Untarnished UI.",
        "czech": "Skyrim: rámeček s uzlovým vzorem - stejný severský ornament, jaký používá téma Skyrim v rámci menu, vykreslený kolem mapy. Výchozí hodnota. Untarnished: jednoduchá jedna linka v barvě off-white z Untarnished UI.",
    },
    "LMU_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "LMU_LogLevel": {
        "japanese": "ログレベル", "korean": "로그 레벨", "chinese": "日志级别", "russian": "Уровень журнала",
        "german": "Protokollstufe", "french": "Niveau de journal", "spanish": "Nivel de registro",
        "italian": "Livello di log", "polish": "Poziom logowania", "czech": "Úroveň logování",
    },
    "LMU_LogLevel_Trace": {
        "japanese": "トレース", "korean": "추적", "chinese": "跟踪", "russian": "Трассировка", "german": "Trace",
        "french": "Trace", "spanish": "Trace", "italian": "Trace", "polish": "Trace", "czech": "Trace",
    },
    "LMU_LogLevel_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "LMU_LogLevel_Info": {
        "japanese": "情報", "korean": "정보", "chinese": "信息", "russian": "Информация", "german": "Info",
        "french": "Infos", "spanish": "Información", "italian": "Informazioni", "polish": "Informacje", "czech": "Informace",
    },
    "LMU_LogLevel_Warning": {
        "japanese": "警告", "korean": "경고", "chinese": "警告", "russian": "Предупреждение", "german": "Warnung",
        "french": "Avertissement", "spanish": "Advertencia", "italian": "Avviso", "polish": "Ostrzeżenie", "czech": "Varování",
    },
    "LMU_LogLevel_Error": {
        "japanese": "エラー", "korean": "오류", "chinese": "错误", "russian": "Ошибка", "german": "Fehler",
        "french": "Erreur", "spanish": "Error", "italian": "Errore", "polish": "Błąd", "czech": "Chyba",
    },
    "LMU_LogLevel_Critical": {
        "japanese": "重大", "korean": "치명적", "chinese": "严重", "russian": "Критическая",
        "german": "Kritisch", "french": "Critique", "spanish": "Crítico", "italian": "Critico",
        "polish": "Krytyczny", "czech": "Kritická",
    },
    "LMU_LogLevel_Off": {
        "japanese": "オフ", "korean": "끄기", "chinese": "关闭", "russian": "Отключено", "german": "Aus",
        "french": "Désactivé", "spanish": "Desactivado", "italian": "Disattivato", "polish": "Wyłączone", "czech": "Vypnuto",
    },
    "LMU_HelpLogLevel": {
        "japanese": "ログに即座に適用されます。",
        "korean": "로그에 즉시 적용됩니다.",
        "chinese": "立即应用于日志。",
        "russian": "Применяется к журналу немедленно.",
        "german": "Wird sofort auf das Log angewendet.",
        "french": "S'applique immédiatement au journal.",
        "spanish": "Se aplica de inmediato al registro.",
        "italian": "Si applica immediatamente al log.",
        "polish": "Stosowane natychmiast do logu.",
        "czech": "Použije se okamžitě na log.",
    },
    "LMU_SaveBtn": {
        "japanese": "保存", "korean": "저장", "chinese": "保存", "russian": "Сохранить", "german": "Speichern",
        "french": "Enregistrer", "spanish": "Guardar", "italian": "Salva", "polish": "Zapisz", "czech": "Uložit",
    },
    "LMU_StatusSaved": {
        "japanese": "設定を保存しました。", "korean": "설정을 저장했습니다.", "chinese": "设置已保存。",
        "russian": "Настройки сохранены.", "german": "Einstellungen gespeichert.", "french": "Paramètres enregistrés.",
        "spanish": "Ajustes guardados.", "italian": "Impostazioni salvate.", "polish": "Ustawienia zapisane.",
        "czech": "Nastavení uložena.",
    },
    "LMU_StatusSaveFail": {
        "japanese": "INIの保存に失敗しました。理由はログを確認してください。",
        "korean": "INI를 저장할 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法保存 INI。请查看日志了解原因。",
        "russian": "Не удалось сохранить INI. Причина — в журнале.",
        "german": "Die INI konnte nicht gespeichert werden. Der Grund steht im Log.",
        "french": "Impossible d'enregistrer l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo guardar el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile salvare l'INI. Consulta il log per il motivo.",
        "polish": "Nie można zapisać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze uložit INI. Důvod najdete v logu.",
    },
    "LMU_HelpSave": {
        "japanese": "上記のすべての設定をプラグインのINIに書き込みます。コメントと無関係なキーはそのまま残ります。",
        "korean": "위의 모든 설정을 플러그인의 INI에 다시 기록합니다. 주석과 관련 없는 키는 그대로 유지됩니다.",
        "chinese": "将以上所有设置写回插件的 INI。注释和无关的键保持不变。",
        "russian": "Записывает каждую настройку выше обратно в INI плагина. Комментарии и посторонние ключи остаются без изменений.",
        "german": "Schreibt jede obige Einstellung zurück in die INI des Plugins. Kommentare und nicht verwandte Schlüssel bleiben unverändert.",
        "french": "Réécrit chaque paramètre ci-dessus dans le fichier INI du plugin. Les commentaires et les clés non liées restent inchangés.",
        "spanish": "Vuelve a escribir cada ajuste anterior en el INI del plugin. Los comentarios y las claves no relacionadas quedan intactos.",
        "italian": "Riscrive ogni impostazione sopra nell'INI del plugin. I commenti e le chiavi non correlate restano intatti.",
        "polish": "Zapisuje każde powyższe ustawienie z powrotem do pliku INI wtyczki. Komentarze i niezwiązane klucze pozostają bez zmian.",
        "czech": "Zapíše každé výše uvedené nastavení zpět do INI pluginu. Komentáře a nesouvisející klíče zůstanou nezměněny.",
    },
    "LMU_ReloadBtn": {
        "japanese": "INIから再読み込み", "korean": "INI에서 다시 불러오기", "chinese": "从 INI 重新加载",
        "russian": "Перезагрузить из INI", "german": "Aus INI neu laden", "french": "Recharger depuis l'INI",
        "spanish": "Recargar desde el INI", "italian": "Ricarica dall'INI", "polish": "Wczytaj ponownie z INI",
        "czech": "Znovu načíst z INI",
    },
    "LMU_StatusReloaded": {
        "japanese": "INIから設定を再読み込みしました。", "korean": "INI에서 설정을 다시 불러왔습니다.",
        "chinese": "已从 INI 重新加载设置。", "russian": "Настройки перезагружены из INI.",
        "german": "Einstellungen aus der INI neu geladen.", "french": "Paramètres rechargés depuis l'INI.",
        "spanish": "Ajustes recargados desde el INI.", "italian": "Impostazioni ricaricate dall'INI.",
        "polish": "Ustawienia wczytane ponownie z INI.", "czech": "Nastavení znovu načtena z INI.",
    },
    "LMU_StatusReloadFail": {
        "japanese": "INIの読み込みに失敗しました。理由はログを確認してください。",
        "korean": "INI를 읽을 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法读取 INI。请查看日志了解原因。",
        "russian": "Не удалось прочитать INI. Причина — в журнале.",
        "german": "Die INI konnte nicht gelesen werden. Der Grund steht im Log.",
        "french": "Impossible de lire l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo leer el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile leggere l'INI. Consulta il log per il motivo.",
        "polish": "Nie można odczytać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze přečíst INI. Důvod najdete v logu.",
    },
    "LMU_HelpReload": {
        "japanese": "最後の保存以降にここで行った変更をすべて捨て、ディスクからINIを再読み込みします。ファイルを手動で編集した変更も取り込みます。",
        "korean": "마지막 저장 이후 여기서 만든 변경 사항을 모두 버리고 디스크에서 INI를 다시 읽습니다. 파일을 직접 편집한 내용도 반영됩니다.",
        "chinese": "放弃自上次保存以来在此处所做的任何更改,并从磁盘重新读取 INI。也会读取手动编辑该文件所做的更改。",
        "russian": "Отбрасывает все изменения, сделанные здесь с последнего сохранения, и заново считывает INI с диска. Также подхватывает изменения, сделанные вручную в файле.",
        "german": "Verwirft jede hier seit dem letzten Speichern vorgenommene Änderung und liest die INI erneut von der Festplatte. Übernimmt auch von Hand vorgenommene Änderungen an der Datei.",
        "french": "Annule tout changement effectué ici depuis le dernier enregistrement et relit l'INI depuis le disque. Reprend aussi les modifications faites manuellement dans le fichier.",
        "spanish": "Descarta cualquier cambio hecho aquí desde el último guardado y vuelve a leer el INI desde el disco. También recoge los cambios hechos a mano en el archivo.",
        "italian": "Scarta ogni modifica fatta qui dall'ultimo salvataggio e rilegge l'INI dal disco. Recupera anche le modifiche fatte a mano al file.",
        "polish": "Odrzuca wszelkie zmiany wprowadzone tutaj od ostatniego zapisu i ponownie odczytuje INI z dysku. Uwzględnia też zmiany wprowadzone ręcznie w pliku.",
        "czech": "Zahodí všechny změny provedené zde od posledního uložení a znovu načte INI z disku. Zohlední i změny provedené ručně v souboru.",
    },
    "LMU_RestoreBtn": {
        "japanese": "既定値に戻す", "korean": "기본값으로 복원", "chinese": "恢复默认值", "russian": "Восстановить умолч.",
        "german": "Standard wiederherstellen", "french": "Restaurer les valeurs par défaut",
        "spanish": "Restaurar valores predeterminados", "italian": "Ripristina i valori predefiniti",
        "polish": "Przywróć wartości domyślne", "czech": "Obnovit výchozí",
    },
    "LMU_StatusRestored": {
        "japanese": "既定値に戻しました。保存を押して確定してください。",
        "korean": "기본값으로 복원했습니다. 유지하려면 저장을 누르세요.",
        "chinese": "已恢复默认值。按保存以保留它们。",
        "russian": "Значения по умолчанию восстановлены. Нажмите «Сохранить», чтобы закрепить их.",
        "german": "Standardwerte wiederherstellt. Drücke Speichern, um sie zu behalten.",
        "french": "Valeurs par défaut restaurées. Appuyez sur Enregistrer pour les conserver.",
        "spanish": "Valores predeterminados restaurados. Pulsa Guardar para conservarlos.",
        "italian": "Valori predefiniti ripristinati. Premi Salva per conservarli.",
        "polish": "Przywrócono wartości domyślne. Naciśnij Zapisz, aby je zachować.",
        "czech": "Výchozí hodnoty obnoveny. Stiskněte Uložit, abyste je zachovali.",
    },
    "LMU_HelpRestore": {
        "japanese": "すべての設定を新規インストール時の値に戻します。保存ボタンを押すまで何も書き込まれません。",
        "korean": "모든 설정을 새로 설치했을 때의 값으로 되돌립니다. 저장 버튼을 누르기 전까지는 아무것도 기록되지 않습니다.",
        "chinese": "将每个设置恢复为全新安装时的值。在你按下保存按钮之前,不会写入任何内容。",
        "russian": "Возвращает каждую настройку к значению, которое было бы при свежей установке. Ничего не записывается, пока вы не нажмёте кнопку «Сохранить».",
        "german": "Setzt jede Einstellung auf den Wert zurück, den sie bei einer frischen Installation hätte. Nichts wird geschrieben, bis du auf die Schaltfläche Speichern drückst.",
        "french": "Remet chaque paramètre à sa valeur d'une installation neuve. Rien n'est écrit avant que vous n'appuyiez sur le bouton Enregistrer.",
        "spanish": "Devuelve cada ajuste al valor que tendría en una instalación nueva. No se escribe nada hasta que pulses el botón Guardar.",
        "italian": "Riporta ogni impostazione al valore che avrebbe in un'installazione nuova. Non viene scritto nulla finché non premi il pulsante Salva.",
        "polish": "Przywraca każde ustawienie do wartości z nowej instalacji. Nic nie zostaje zapisane, dopóki nie naciśniesz przycisku Zapisz.",
        "czech": "Vrátí každé nastavení na hodnotu, jakou by mělo při čerstvé instalaci. Nic se nezapíše, dokud nestisknete tlačítko Uložit.",
    },
    "LMU_Intro": {
        "japanese": "ほとんどの設定は変更するとすぐに適用されます。次回プレイ時にも残すには保存を押してください。",
        "korean": "대부분의 설정은 변경하는 즉시 적용됩니다. 다음에 플레이할 때도 유지하려면 저장을 누르세요.",
        "chinese": "大多数设置在你更改后会立即生效。按保存可在下次游玩时保留它们。",
        "russian": "Большинство настроек применяются сразу же после изменения. Нажмите «Сохранить», чтобы они остались и в следующий раз.",
        "german": "Die meisten Einstellungen wirken sofort, sobald du sie änderst. Drücke Speichern, um sie für das nächste Mal zu behalten.",
        "french": "La plupart des paramètres s'appliquent dès que vous les modifiez. Appuyez sur Enregistrer pour les garder la prochaine fois.",
        "spanish": "La mayoría de los ajustes se aplican en cuanto los cambias. Pulsa Guardar para conservarlos la próxima vez que juegues.",
        "italian": "La maggior parte delle impostazioni si applica non appena le modifichi. Premi Salva per conservarle per la prossima partita.",
        "polish": "Większość ustawień obowiązuje natychmiast po ich zmianie. Naciśnij Zapisz, aby zachować je na następną rozgrywkę.",
        "czech": "Většina nastavení se použije okamžitě po jejich změně. Stiskněte Uložit, abyste je zachovali pro příští hraní.",
    },
}


def write_translation_file(path, entries):
    lines = []
    for key, text in entries.items():
        escaped = text.replace("\r\n", "\n").replace("\n", "\\n")
        lines.append(f"${key}\t{escaped}")
    body = "\r\n".join(lines) + "\r\n"
    data = b"\xff\xfe" + body.encode("utf-16-le")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main():
    keys, order = read_keys()
    missing_translation_keys = [k for k in order if k not in TRANSLATIONS]
    if missing_translation_keys:
        raise RuntimeError(f"no translations held for keys found in source: {missing_translation_keys}")
    extra_translation_keys = [k for k in TRANSLATIONS if k not in keys]
    if extra_translation_keys:
        raise RuntimeError(f"translations held for keys no longer in source: {extra_translation_keys}")

    out_dir = os.path.join(REPO, "dist", "Interface", "Translations")
    english = {k: keys[k] for k in order}
    write_translation_file(os.path.join(out_dir, "LocalMapUpgrade_english.txt"), english)
    print(f"english: {len(english)} keys")

    for lang in LANGS[1:]:
        translated = {k: TRANSLATIONS[k][lang] for k in order}
        write_translation_file(os.path.join(out_dir, f"LocalMapUpgrade_{lang}.txt"), translated)
        print(f"{lang}: {len(translated)} keys written")


if __name__ == "__main__":
    main()
