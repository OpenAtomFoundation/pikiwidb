start_server {tags {"string manifestingest"}} {

    # 测试参数数量错误
    test {manifestingest: wrong number of args} {
        catch {r manifestingest} e
        string match "*wrong number of arguments*" [string tolower $e]
    } {1}

    # 测试正常 ingest
    test {manifestingest: ingest success from prepared manifest} {
        r manifestingest manifest_1759125977953253000_part0.proto
    } {OK}

    # 数据校验
    test {manifestingest: verify kv from data_0.json - 1} {
        r get key_011912412414
    } {value_154661251424274129}

    test {manifestingest: verify kv from data_0.json - 2} {
        r get key_014310660701
    } {value_131919586971241288}

    test {manifestingest: verify kv from data_0.json - 3} {
        r get key_059551467315
    } {value_108109118129921469}

    test {manifestingest: verify kv from data_0.json - 4} {
        r get key_100101147501
    } {value_656914412392917379}
}
