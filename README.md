Этот проект представляет собой асинхронный c++ HTTP-сервер написанный при помощи библеотеки Boost. 
Основные его концепции:
1)Обработка HTTP-запросов
  (1)Сервер принимает HHTP запросы и анализирует их содержимое.
  (2)Сервер парсит запросы на:
    1)Заголовки.
    2)Методы.
    3)URL.
    4)Тела.
  (3)Сервер так же обслуживание статические файлы.
  
2)Aсинхронная архитектура 
  (1)Сервер написан на библеотеке Boost, которая дает ему асинхронное управление сокетами и запросами.
  (2)Сама асинхронная модель позволяет обрабатывать серверу большое количество запросов и соединений, при этом не блокируя основной.

3)Многопоточность и производительность
  (1)используются пул потоки для обработки HTTP-запросов.
  (2)Эффективно распределяется нагрузка между ядрами процессора и не ограничивает его в производительности.
  
4)Потокобезопасность
  (1)Используются атомарные переменные для подсчета активных соединений и запросов.
  (2)RAII для упрвления соединениями.
  (3)Из-за использования пул потоков так же увеличивается потокбезопасность.
  (4)В сервере используется асинхронная обработка запросов.
  (5)Потокобезопасное кеширование.
  (6)В сервере есть возможность предотвращения перегрузки.
  (7)Управление логами.
  (8)Зашита от сбоев.

5)Маршрутизация
  (1)Сервер регистрирует обработчики для различных URL-адресов.
  (2)Сервер обрабатывает маршруты.

6)Логирование
 (1)Все логи записываются в файл в текстовом формате.
  (2)Сервер логирует такие данные как:
    1)Дата и время запроса.
    2)IP клиентов.
    3)Содержимое самого запроса.
 
7)Конфигурируемость
  (1)Сервер использует параметры командной строки, что позволяет пользователям задавать различные параметры при запуске программы.
  (2)Логирование и динамическая настройка потоков.
  
8)Управление соединениями
  (1)Создание и принятие соединений
  (2)Возможность управления количеством активных соединений 
  (3)Ограничение максимального количества соединений
  (4)Управление открытием и закрытием соединений

9)Обработка ошибок
  (1)Обработка ошибок при соединении и асинхронных операциях
  (2)Обработка ошибок в логике запроса
  (3)Обработка ошибок при работе с файлами
  (4)Обработка HTTP ошибок
  (5)Логирование ошибок


10)Кеширование
  (1)Сервер реализует кеширование для ускорения работы с запросами на статические файлы, что повышает производительность и снижает задержки при обработке запросов.
  (2)Максимальный размер самого кэша задается в коде (по умолчанию равен 50 файлов), при исбыточности размера кеша новые файлы не добавляются.
