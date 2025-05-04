all:
	pio run -e esp12e-ota -t upload

db:
	pio run -e esp12e-ota -t compiledb
