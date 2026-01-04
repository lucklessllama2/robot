ARCH ?= $(shell cat /proc/sys/kernel/arch)
BUILD ?= build
CFLAGS ?= -Wall -Wextra -g -O0
CPPFLAGS ?= -g -O0 -D$(ARCH)=1

COMMON_SRC := i2cdev.cpp pca9685.cpp pwm_mock.cpp servo.cpp state.cpp status.cpp
COMMON_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(COMMON_SRC))

all: calibrate limit servo_test

info:
	echo "ARCH=$(ARCH)"

-include $(BUILD)/*.d

$(BUILD)/%.o : %.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $@.d -o $@ -c $<


.PHONY: clean transfer
clean:
	rm -rf servo_test calibrate limit $(BUILD)

transfer: clean
	rsync -avzP . dogo@192.168.1.221:~/i2c-servo	

pull: clean
	scp dogo@192.168.1.221:~/i2c-servo/* .
# scp * dogo@192.168.1.221:~/i2c-servo

.PHONY: crun cdebug
gamma : $(COMMON_OBJ) $(BUILD)/gamma.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)

grun: gamma
	./$<

gdebug: gamma
	gdb ./$< -x .gdbinit


.PHONY: crun cdebug
calibrate : $(COMMON_OBJ) $(BUILD)/calibrate.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)

crun: calibrate
	./$@

cdebug: calibrate
	gdb ./$< -x .gdbinit


.PHONY: lrun ldebug
limit : $(COMMON_OBJ) $(BUILD)/limit.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)

lrun: limit
	./$<

ldebug: calibrate
	gdb ./$< -x .gdbinit


.PHONY: srun sdebug
servo_test : $(COMMON_OBJ) $(BUILD)/servo_test.o
	$(CXX) $^ -o $@ -g $(LDFLAGS)

srun: servo_test
	./$<

sdebug: servo_test
	gdb ./$< -x .gdbinit
