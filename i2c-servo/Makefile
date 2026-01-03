ARCH ?= $(shell cat /proc/sys/kernel/arch)
BUILD ?= build
CFLAGS ?= -Wall -Wextra -g -O0
CPPFLAGS ?= -g -O0 -D$(ARCH)=1

COMMON_SRC := i2cdev.cpp pca9685.cpp pwm_mock.cpp servo.cpp status.cpp
COMMON_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(COMMON_SRC))

all: pca_test servo_test

info:
	echo "ARCH=$(ARCH)"

-include $(BUILD)/*.d

$(BUILD)/%.o : %.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $@.d -o $@ -c $<

pca_test : $(COMMON_OBJ) $(BUILD)/state.o $(BUILD)/main.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)

servo_test: $(COMMON_OBJ) $(BUILD)/servo_test.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)


.PHONY: run debug
run: pca_test
	./pca_test

debug: pca_test
	gdb ./pca_test -x .gdbinit


.PHONY: test debug_test
debug_test: servo_test
	gdb ./servo_test  -x .gdbinit

test: servo_test
	./servo_test


.PHONY: clean transfer
clean:
	rm -rf servo_test pca_test $(BUILD)

transfer: clean
	rsync -avzP . dogo@192.168.1.221:~/i2c-servo	

pull: clean
	scp dogo@192.168.1.221:~/i2c-servo/* .

# scp * dogo@192.168.1.221:~/i2c-servo


