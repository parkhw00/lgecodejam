
CFLAGS += -O2

#TEST += a
#TEST += b
TEST += c
#TEST += d

RUN = $(TEST:%=test_%)

all: $(RUN)

define run
test_$(1): $(1) $(1).in
	./$(1) < $(1).in

$(1): $(1).c

endef

$(eval $(foreach t,$(TEST),$(call run,$(t))))

