.PHONY: clean All

All:
	@echo "----------Building project:[ IM-DSSE - Debug ]----------"
	@cd "IM-DSSE" && "$(MAKE)" -f  "IM-DSSE.mk"
clean:
	@echo "----------Cleaning project:[ IM-DSSE - Debug ]----------"
	@cd "IM-DSSE" && "$(MAKE)" -f  "IM-DSSE.mk" clean
