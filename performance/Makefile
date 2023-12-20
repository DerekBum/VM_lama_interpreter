TESTS=$(sort $(basename $(wildcard *.lama)))

LAMAC=lamac
LAMA_C=../build/lama_c_interpreter

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.lama
	@echo "Lamac interpreter:"
	@echo 0 | LAMA=../ `which time` -f "$@\t%U" $(LAMAC) -s $< > $@.log
	@echo "Lama_C interpreter:"
	@echo 0 | `which time` -f "$@\t%U" sh -c 'lamac -b $< > $@.bc && $(LAMA_C) $@.bc'

clean:
	$(RM) test*.log *.s *~ $(TESTS) *.i
