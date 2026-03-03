CCHECKS = -fsanitize=address,leak,undefined,bounds -fsanitize-address-use-after-scope
CWARNINGS = -W -Wall -Wno-switch -Wextra -Wuninitialized -Wunreachable-code
CFLAGS = $(CWARNINGS) $(CCHECKS) -std=c99

#############################################################

BIN_DIR  := ./build
SRC_DIR  := .

.PHONY: Cdecl
Cdecl: SRC_LIST := $(SRC_DIR)/Cdecl.c
Cdecl: build run clean

#-----------------
.PHONY: build 
build: 
	@mkdir -p $(BIN_DIR)
	@$(foreach src,$(SRC_LIST), \
		echo "[INFO] building '$(src)'"; \
		gcc '$(src)' $(CFLAGS) -o '$(BIN_DIR)/$(notdir $(basename $(src)))'; \
	)

.PHONY: run 
run:
	@$(foreach src,$(SRC_LIST), \
		echo; echo [INFO] $(BIN_DIR)/$(notdir $(basename $(src))):;  $(BIN_DIR)/$(notdir $(basename $(src))) "$(INPUT)"; echo; \
	)

.PHONY: clean 
clean:
	@echo [INFO] removing build files
	@if [ -n "$(BIN_DIR)" ] && [ "$(BIN_DIR)" != "." ]; then \
		echo "[INFO] removing directory '$(BIN_DIR)'"; \
		rm -r "$(BIN_DIR)"; \
	else \
		$(foreach src,$(SRC_LIST), \
			rm $(BIN_DIR)/$(notdir $(basename $(src))); \
		) \
	fi
