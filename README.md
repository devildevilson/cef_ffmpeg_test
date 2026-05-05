# cef_ffmpeg_test

Чекаю как соединить CEF + FFMPEG + OPENGL

### Описание
Экран в приложении делится на 3 части, верхняя половина полностью отводится под изображение полученное от CEF, нижняя половина делится на две и там проигрываются 2 видео с помощью FFMPEG. Что я понял? OpenGL наверное еще видел как люди лампочки изобретают и нужно его уже давно меня на что то более приличное. CEF огромен и при этом умудряется быть капризным и похоже что может не запуститься с первого раза. FFMPEG как всегда хорош - понятные интерфейсы и более менее понятный процесс того как из видоса извлекать кадр, даже захотелось добавить его в какой нибудь из своих проектов

### Архитектура
Приложение строится вокруг 3 частей:
1. Источники картинок - CEF + FFMPEG
2. Клей в виде `resource_manager` - предоставляет актуальные буферы для источников
3. Рендер на OpenGL - забирает новые картиночки из `resource_manager`
- `resource_manager` делался с прицелом на возможную многопоточность, но к сожалению сильно долго провозился с зависимостями к проекту и не успел добавить на текущий момент
- Потенциально можно добавить любое количество CEF и FFMPEG источников, по умолчанию например два FFMPEG источника
- Нужно добавить стандартный конфиг проекта, откуда данные подгрузятся
- Я сделал мерзковатый способ выйти из приложения через глобальный счетчик
- Проект собирается на C++20 из-за CEF

## сборка для винды
Бинарники FFMPEG и CEF скачаются сами, GLEW скачается сам, требуется visual studio 2022 + cmake 3.30+ 

## сборка для линуха
Бинарники качать не хочу, так что зависимости нужно поставить из пакетного менеджера, но CEF похоже что ставится только из бинарников. Для того чтобы поставить cmake 3.30+ нужно подрубить сторонний репозиторий, смотреть тут https://apt.kitware.com/
```
sudo apt install -y build-essential cmake git libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev libglew-dev libgbm-dev libx11-dev libxcomposite-dev libxcursor-dev libxdamage-dev libxext-dev libxfixes-dev libxi-dev libxrandr-dev libxrender-dev libxss-dev libxtst-dev libxkbcommon-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libavdevice-dev libavfilter-dev libpostproc-dev libnss3 libatk1.0-0 libatk-bridge2.0-0 libcups2 libdrm2 libgtk-3-0 libpango-1.0-0 pkg-config libgtk-3-dev libglib2.0-dev
```
Скачать актуальный cef_binary средствами CMAKE не получилось, скачайте его wget'ом в папку external
```
wget https://cef-builds.spotifycdn.com/cef_binary_147.0.10%2Bgd58e84d%2Bchromium-147.0.7727.118_linux64.tar.bz2 -O ./external/cef_binary.tar.bz2
```
К сожалению мне не получилось отменить сборку тестов у CEF. Сборка тестов у CEF падает ровно в одном месте. Находясь в папке `build` поправте файлик
```
_deps/cef-src/tests/ceftests/base/ref_counted_unittest.cc
```
Там `std::move` вызывается у переменной на самого себя

### Ресурсы
- test1: License:Creative Commons Attribution 3.0 https://www.youtube.com/watch?v=o9tMUcnN6_0
- test2: Лицензия Creative Commons – Attribution (разрешено повторное использование) https://www.youtube.com/watch?v=Byo-lcR6Bmw
