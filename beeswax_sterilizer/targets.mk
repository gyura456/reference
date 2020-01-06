

Debug:all

flash:
	st-flash write build/$(PROJECT).bin 0x8000000

cleanDebug: clean all

doc:
	$(MAKE) -C doc

createtask:
	echo -n "taskname:"; \
	read TASK_NAME ;     \
	if [ -z "$$TASK_NAME" ];  then  \
	    echo "need set name of task"; exit 1; \
	else  \
	    echo "create"; \
	  ../createtask $$TASK_NAME; \
	fi

.PHONY: clean cleanDebug doc killterm



