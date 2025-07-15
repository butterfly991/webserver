# 📦 ServerProject — Асинхронный HTTP-сервер на C++ с поддержкой статики и роутинга

## Описание

ServerProject — это легковесный асинхронный HTTP-сервер, написанный на C++ с использованием Boost.Asio.
Сервер поддерживает:
- отдачу статических файлов,
- регистрацию пользовательских маршрутов (роутинг),
- логирование запросов,
- сбор статистики,
- многопоточную обработку.

---

## 🚀 Быстрый старт

### Требования

- C++17
- [CMake ≥ 3.10](https://cmake.org/)
- [Boost (system, filesystem, program_options)](https://www.boost.org/)

### Сборка

```sh
git clone https://github.com/yourname/ServerProject.git
cd ServerProject
mkdir build && cd build
cmake ..
make
```

### Запуск

```sh
./ServerProject --port 8080 --threads 4 --static static
```

**Параметры командной строки:**
- `--port` — порт для сервера (по умолчанию 8080)
- `--threads` — количество потоков (по умолчанию 4)
- `--static` — директория со статическими файлами (по умолчанию "static")

---

## 🗂️ Структура проекта

```
.
├── include/                # Заголовочные файлы (классы сервера, обработчика, RAII, статистики)
├── src/                    # Исходники (реализация классов и main)
├── static/                 # Статические файлы (index.html, favicon.ico)
├── server_log.txt          # Лог-файл запросов
├── CMakeLists.txt          # Конфигурация сборки
└── README.md               # Документация
```

---

## ⚙️ Пример использования

1. Откройте браузер и перейдите на [http://localhost:8080/](http://localhost:8080/)
2. Сервер отдаст файл `static/index.html`.
3. Для favicon используйте `/favicon.ico`.

---

## 🧩 Расширение функционала

### Регистрация пользовательских маршрутов

В файле `src/main.cpp` (или в другом месте после создания сервера):

```cpp
server._request_handler_.RegisterRoute("/hello", 
  [](const std::string& body, 
     const std::map<std::string, std::string>& headers, 
     const std::string& method) {
    return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!";
});
```

---

## 📊 Логирование и статистика

- Все запросы логируются в `server_log.txt`.
- В статистике сервера учитываются:
  - Количество обработанных запросов
  - Количество активных потоков
  - Время запуска сервера

---

## ❓ FAQ

**Q:** Как добавить свой обработчик?  
**A:** Используйте метод `RegisterRoute` у `ReQuestHandler`.

**Q:** Как изменить директорию со статикой?  
**A:** Передайте путь через параметр `--static`.

**Q:** Какие файлы отдаются по умолчанию?  
**A:** `index.html` для `/`, `favicon.ico` для `/favicon.ico`.

---

## 🛠️ Зависимости

- Boost.Asio
- Boost.Filesystem
- Boost.ProgramOptions

---
