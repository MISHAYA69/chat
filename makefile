# Makefile для сборки TCP клиента и сервера под Windows
# Компилятор (используйте нужный)
# Для MinGW:
CXX = g++
# Для Visual Studio (раскомментируйте):
# CXX = cl

# Флаги компиляции
CXXFLAGS = -Wall -O2
# Для Visual Studio добавьте:
# CXXFLAGS = /EHsc /O2

# Линковка с библиотекой сокетов
LDFLAGS = -lws2_32

# Исходные файлы
CLIENT_SRC = client.cpp
SERVER_SRC = server.cpp

# Исполняемые файлы
CLIENT_TARGET = client.exe
SERVER_TARGET = server.exe

# Правило по умолчанию
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Сборка клиента
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Сборка сервера
$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Очистка
clean:
	del /Q $(CLIENT_TARGET) $(SERVER_TARGET) 2>nul || rm -f $(CLIENT_TARGET) $(SERVER_TARGET)

# Запуск клиента (можно передать имя)
run-client: $(CLIENT_TARGET)
	$(CLIENT_TARGET) "$(NAME)"

# Запуск сервера
run-server: $(SERVER_TARGET)
	$(SERVER_TARGET)

# Псевдо-цели
.PHONY: all clean run-client run-server