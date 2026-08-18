[English](BUILDING.md) | [Українська](BUILDING.uk.md) | **[Русский](BUILDING.ru.md)**

# Сборка из исходного кода

## Что нужно

Нужен **devkitPro** с тулчейном для Switch (`devkitA64` + `libnx`), а также
**libjpeg-turbo** для Switch (нужна для декодирования JPEG-иконок игр,
встроенных в `NsApplicationControlData`).

### Windows

1. Скачайте и запустите [графический установщик devkitPro](https://github.com/devkitPro/installer/releases)
   для Windows. Когда он спросит, какие платформы установить, обязательно
   поставьте галочку на **Switch development**.
2. После завершения откройте **Пуск → devkitPro → MSYS2** (это встроенная
   оболочка MSYS2 с уже настроенными репозиториями devkitPro — используйте
   именно её, а не обычные `cmd`/PowerShell, для всех шагов ниже).
3. В этом окне MSYS2 установите тулчейн для Switch и libjpeg-turbo:
   ```bash
   pacman -S switch-dev switch-libjpeg-turbo
   ```
4. Закройте и снова откройте окно MSYS2 один раз (чтобы подхватилась
   переменная окружения `DEVKITPRO`, которую выставляет установщик), затем
   проверьте её:
   ```bash
   echo $DEVKITPRO
   ```
   Должно вывести что-то вроде `/c/devkitPro`. Если пусто — установщик
   завершился некорректно, запустите его ещё раз.
5. Далее выполняйте шаги **Сборка** ниже из того же окна MSYS2.

> **Примечание:** `git` не входит в состав MSYS2 devkitPro по умолчанию.
> Можно либо установить его там (`pacman -S git`), либо просто скачать
> репозиторий ZIP-архивом с GitHub (**Code → Download ZIP**), распаковать
> его и зайти в эту папку из MSYS2 вместо `git clone`.

### Linux / macOS

Установите через пакетный менеджер [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman):
```bash
sudo dkp-pacman -S switch-dev switch-libjpeg-turbo
```
Убедитесь, что переменная `DEVKITPRO` выставлена в окружении (установщик
делает это сам; проверьте через `echo $DEVKITPRO`, при необходимости
перелогиньтесь сразу после установки, если она пустая).

## Сборка

```bash
git clone https://github.com/Rexead/GameTime-Tracker.git
cd GameTime-Tracker
make
```

Это создаст `GameTime-Tracker.nro` в корне проекта (имя берётся из имени
папки — переименуйте папку, если хотите другое имя `.nro`).

`make clean` удаляет папку `build/` и все артефакты сборки.

## Структура проекта

```
GameTime-Tracker/
├── source/          # .cpp файлы (компилируются Makefile)
│   ├── main.cpp         # основной цикл, обработка ввода, состояние экранов
│   ├── game_loader.cpp  # запросы к NS/PDM, декодирование JPEG-иконок, фоновый поток загрузки
│   ├── renderer.cpp     # отрисовка во framebuffer (список, детали, шкала загрузки, растровый шрифт)
│   └── utils.cpp        # форматирование времени игры
├── include/
│   └── gametime.h       # общие структуры, константы интерфейса, объявления функций
└── Makefile
```

## Установка на консоль

Смотрите раздел [Установка](README.ru.md#установка) в README — тот же
`.nro`, который вы собрали здесь, кидается на SD-карту так же, как и
сборка из Releases.
