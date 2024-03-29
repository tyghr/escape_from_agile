Игра «Программист против менеджеров». 2D Game. Dev vs managers.  

---
![game](escape_from_agile.png)
---
How to install and launch:

```sh
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

make
./app

```
---

Описание:
- Игровое поле, генерируется каждый раз, с рандомными препятствиями и покрытием (пока только текстура).
- Герой - разработчик, свободно перемещается по полю, и пытается написать код.
- Также присутствуют менеджеры, которые пристают к герою с постоянными вопросами о сроках.
- Менеджер появляется в случайном месте на поле и идет в сторону разработчика, по кратчайшему пути.
- Писать код у разработчика получается только когда он не перемещается и когда рядом нет менеджеров.
- Вверху отображается счетчик написания (PROG, зависит от DIST).
- Со временем менеджеров становится больше, их скорость также растет.
- Также вверху отображается расстояние до ближайшего менеджера (DIST).
- Если менеджер находит разработчика, помимо приостановки написания кода, у разработчика тратится ментальное здоровье (индикатор HEAL).
- При достаточном отдалении, здоровье восстанавливается.
- Если здоровье упадет до 0, игра окончена.

TODO:
- рефакторинг.
- добавить highscores.
- добавить эффекты, например в зависимости от близости монстра.
- добавить менеджерам состояние "неизвестности маршрута" до героя. Т.е. движение до последней клетки, где ктото из менеджеров видел разработчика. Иначе рандомные передвижения.
- добавить вариативность препятствий.
- добавить аудио эффекты и фон.
- добавить интерактивные препятствия.
- переходы в соседние комнаты.
