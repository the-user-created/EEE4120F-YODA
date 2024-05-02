PACKAGE_NAME := YODA Project

.PHONY: help
help: ## This help dialog.
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | \
	awk 'BEGIN {FS = "#"} {printf "%-30s %s\n", $$1, $$3}' | sed "s/\$$(PACKAGE_NAME)/$(PACKAGE_NAME)/g"

.PHONY: install-macos
install-macos: ## Installs with all devtools, for development.
	brew install pre-commit && \
	brew install llvm uncrustify cppcheck include-what-you-use oclint

.PHONY: install-dev
install-dev: ## Installs with all devtools, for development.
	pre-commit install && \
	pre-commit autoupdate

.PHONY: pc
pc: ## Executes pre-commit hook on all files.
	pre-commit run --all-files
