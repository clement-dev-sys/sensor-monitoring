CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
LDLIBS  := -lsqlite3 -ljson-c -lpaho-mqtt3c

TARGET  := sensor-monitoring-server
SRCS    := sensor-monitoring-server.c db.c measurement.c mqtt_sub.c log.c
OBJS    := $(SRCS:.c=.o)
DEPS    := db.h measurement.h mqtt_sub.h info_config.h log.h

.PHONY: all run clean distclean help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	../sensor-monitoring-database/purge_db.sh

help:
	@echo "Commandes disponibles:"
	@echo "  make             - Compiler le projet"
	@echo "  make run         - Compiler et lancer"
	@echo "  make clean       - Nettoyer les objets et le binaire"
	@echo "  make distclean   - Nettoyer les objets, le binaire et purge de la base de données"
	@echo "  make help        - Afficher l'aide"
