# Compiler, compiler and Linker flags.
CC := gcc
CFLAGS := -Ilib/SDL/include -Wall -MMD
LDFLAGS := -Llib/SDL/lib -Lbin/obj 
LDLIBS := -lm -lSDL2main -lSDL2

# Platform conditional compilation
ifeq ($(OS),Windows_NT)
	CFLAGS += -mwindows
	LDLIBS := -lmingw32 $(LDLIBS)
endif

# Source dir, object and dependency file gen dir.
SRCDIR := ./src
OBJDIR := ./bin/obj

# Source, object, dependency file and directory names. 
SRCS := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/**/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.d, $(SRCS))
ODIR := $(sort $(dir $(OBJS)))
EXEC := ./bin/renderer.exe

.PHONY: init run clean

# Build if needed and run the executable produced.
run: $(EXEC)
	$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $(EXEC)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(ODIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

# Ensure obj dirs exist before build
$(ODIR):
	@mkdir -p $@

init:
	@mkdir -p bin
	@cp ./lib/SDL/bin/SDL2.dll ./bin/
	@echo Finished Initialization

clean:
	rm -rf $(OBJDIR)/*

