.PHONY: clean All

All:
	@echo "----------Building project:[ SEU-DSSE - Debug ]----------"
	@cd "SEU-DSSE" && "$(MAKE)" -f  "SEU-DSSE.mk"
clean:
	@echo "----------Cleaning project:[ SEU-DSSE - Debug ]----------"
	@cd "SEU-DSSE" && "$(MAKE)" -f  "SEU-DSSE.mk" clean
