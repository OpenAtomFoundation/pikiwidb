.DEFAULT_GOAL := build-all

export GO111MODULE=on

build-all: generate-version codis-dashboard codis-proxy codis-admin codis-ha codis-fe restore-go-mod

# go1.24's `go mod tidy` rewrites `go 1.23` -> `go 1.23.0` and injects a toolchain
# line, which CentOS7 CI rejects. Restore go.mod/go.sum to the committed state
# after the build so the normalization never lands in version control.
restore-go-mod:
	@git checkout -- go.mod go.sum 2>/dev/null || true

PRJ_ROOT=${CURDIR}

generate-version:
	$(info generate version)
	@mkdir -p ./bin && ./version

codis-deps:
	@go version
codis-dashboard: codis-deps
	$(info build codis-dashboard)
	@cd ${PRJ_ROOT}/cmd/dashboard && go mod tidy && go build -buildvcs=false -o ${PRJ_ROOT}/bin/codis-dashboard .
	@${PRJ_ROOT}/bin/codis-dashboard --default-config > ${PRJ_ROOT}/config/dashboard.toml

codis-proxy: codis-deps
	$(info build codis-proxy)
	@cd ${PRJ_ROOT}/cmd/proxy && go mod tidy && go build -buildvcs=false -o ${PRJ_ROOT}/bin/codis-proxy .
	@${PRJ_ROOT}/bin/codis-proxy --default-config > ${PRJ_ROOT}/config/proxy.toml

codis-admin: codis-deps
	$(info build codis-admin)
	@cd ${PRJ_ROOT}/cmd/admin && go mod tidy && go build -buildvcs=false -o ${PRJ_ROOT}/bin/codis-admin .

codis-ha: codis-deps
	$(info build codis-ha)
	@cd ${PRJ_ROOT}/cmd/ha && go mod tidy && go build -buildvcs=false -o ${PRJ_ROOT}/bin/codis-ha .

codis-fe: codis-deps
	$(info build codis-fe)
	@cd ${PRJ_ROOT}/cmd/fe && go mod tidy && go build -buildvcs=false -o ${PRJ_ROOT}/bin/codis-fe .
	@rm -rf ${PRJ_ROOT}/bin/assets && cp -rf ${PRJ_ROOT}/cmd/fe/assets ./bin/

clean:
	$(info ...Clean Start!)
	@rm -rf bin
