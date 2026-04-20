# PikiwiDB Maintainers

This document lists the maintainers, committers, and notable contributors of the PikiwiDB project.

## Roles

| Role | Description |
|------|-------------|
| **Maintainer** | Has write access to the repository, reviews and merges PRs, makes release decisions, and drives project direction. |
| **Committer** | Active contributor with elevated trust; reviews PRs and contributes significant features or fixes over multiple releases. |
| **Notable Contributor** | Has made meaningful one-off or focused contributions (features, bug fixes, tests, docs, tooling). |

## Maintainers

| GitHub ID | Name | Organization | Email |
|-----------|------|--------------|-------|
| [@chejinge](https://github.com/chejinge) | chejinge | 360 | chejinge@360.cn |
| [@Mixficsol](https://github.com/Mixficsol) | Mixficsol | — | — |
| [@cheniujh](https://github.com/cheniujh) | cheniujh | — | — |

## Committers

Committers have a sustained history of high-quality contributions across multiple releases.

| GitHub ID | Name | Key Contribution Areas |
|-----------|------|------------------------|
| [@wangshao1](https://github.com/wangshao1) | wangshao1 | Floyd storage engine, master-slave replication, monitoring, performance |
| [@buzhimingyonghu](https://github.com/buzhimingyonghu) | buzhimingyonghu | Pika-Operator (backup/recovery, scale-down), Pika-Exporter compatibility |
| [@QlQlqiqi](https://github.com/QlQlqiqi) | QlQl | RedisCache large-key eviction, new compact strategies, CI |
| [@YuCai18](https://github.com/YuCai18) | Yu Cai (蔡煜) | Bug fixes (getrange/setrange crash, zadd, password auth, logging) |
| [@chenbt-hz](https://github.com/chenbt-hz) | chenbt | ZSet command fixes, blob-cache, Codis dashboard, pika_exporter |
| [@XiaoLiang2333](https://github.com/XiaoLiang2333) | DawnBeams | Pika-Operator (kubeblocks upgrade, master-slave mode, log cleanup) |
| [@luky116](https://github.com/luky116) | JayLiu | Redis transactions, ACL, benchmarking, Floyd TTL, Docker docs |
| [@dingxiaoshuai123](https://github.com/dingxiaoshuai123) | dingxiaoshuai | Fast/slow command separation, Codis-proxy monitoring, pika_exporter, Go tests |
| [@lqxhub](https://github.com/lqxhub) | lqxhub | ACL support, FreeBSD compilation, cache flag fixes |
| [@chengyu-l](https://github.com/chengyu-l) | chengyu-l | Codis auto failover, dashboard panic recovery, network metrics, Codis CPU fixes |
| [@baerwang](https://github.com/baerwang) | baerwang | GitHub Actions CI/CD, build caching, release packaging, PR lint |
| [@tsinow](https://github.com/tsinow) | tsinow | Go tests for complex data types and management commands |
| [@saz97](https://github.com/saz97) | saz97 | TCL tests for Geo, HyperLogLog/String type isolation |
| [@u6th9d](https://github.com/u6th9d) | u6th9d | CompactRange command, small-time compaction policy, data type overflow fixes |
| [@ForestLH](https://github.com/ForestLH) | ForestLH | Redis MULTI/EXEC transaction support, master-slave deadlock fix |
| [@Tianpingan](https://github.com/Tianpingan) | Tianpingan | Dynamic flush/compaction thread tuning, work-queue length monitoring |
| [@machinly](https://github.com/machinly) | machinly | Pika Operator cluster auto-scaling, K8s service auto-registration |
| [@hero-heng](https://github.com/hero-heng) | hero-heng | bgsave unix timestamp, dynamic `disable_auto_compactions` support |
| [@baixin01](https://github.com/baixin01) | baixin01 | RocksDB statistics tickers, full-sync data progress metrics |

## Notable Contributors

The following contributors have made meaningful contributions to specific features, bug fixes, tests, or tooling.

| GitHub ID | Contribution Highlights |
|-----------|------------------------|
| [@vacheli](https://github.com/vacheli) | Partition index filtering, disk I/O speed limit (OnlyRead/OnlyWrite/ReadAndWrite), Codis dashboard master status fix |
| [@bigdaronlee163](https://github.com/bigdaronlee163) | BlockCache calculation accuracy fix |
| [@guangkun123](https://github.com/guangkun123) | Pika-port data migration error fix |
| [@longfar-ncy](https://github.com/longfar-ncy) | pksetexat RedisCache consistency fix |
| [@gukj-spel](https://github.com/gukj-spel) | Data race fix in Cmd initialization |
| [@MalikHou](https://github.com/MalikHou) | Dynamic RocksDB Compaction strategy adjustment |
| [@hahahashen](https://github.com/hahahashen) | Rpushx command cache update fix |
| [@KKorpse](https://github.com/KKorpse) | Redis Stream data type support |
| [@sjcsjc123](https://github.com/sjcsjc123) | Large key analysis tool |
| [@JasirVoriya](https://github.com/JasirVoriya) | RocksDB upgrade to v8.7.3 |
| [@HappyUncle](https://github.com/HappyUncle) | Dynamic `max-conn-rbuf-size` parameter support |
| [@Y-Rookie](https://github.com/Y-Rookie) | Pika-Operator namespace support for multi-cluster deployment |
| [@panlei-coder](https://github.com/panlei-coder) | Disable compaction on shutdown for faster exit |
| [@tedli](https://github.com/tedli) | Block read traffic on slave during full replication |
| [@chienguo](https://github.com/chienguo) | Codis `INFO` command support |
| [@Polaris3003](https://github.com/Polaris3003) | Pika Exporter startup-without-params fix |
| [@callme-taota](https://github.com/callme-taota) | RocksDB return value checking in commands |
| [@jettcc](https://github.com/jettcc) | Configuration file parameter loading fix |
| [@ForestLH](https://github.com/ForestLH) | Master-slave replication deadlock fix after flushdb |
| [@xiezheng-XD](https://github.com/xiezheng-XD) | `make -j` parallel build speed improvement |
| [@A2ureStone](https://github.com/A2ureStone) | macOS tools compilation fix |
| [@klboke](https://github.com/klboke) | macOS development environment documentation |
| [@wanghenshui](https://github.com/wanghenshui) | Code quality and miscellaneous fixes |
| [@007gzs](https://github.com/007gzs) | Miscellaneous fixes |
| [@VanessaXWGUO](https://github.com/VanessaXWGUO) | Miscellaneous contributions |
| [@Z-G-H1](https://github.com/Z-G-H1) | Miscellaneous contributions |
| [@pro-spild](https://github.com/pro-spild) | Miscellaneous contributions |

## Emeritus Maintainers

Individuals who have previously served as maintainers and have since stepped back.  
We thank them for their valuable contributions.

<!-- Add emeritus maintainers here when applicable -->

## How to Become a Committer or Maintainer

- **Committer**: Submit high-quality contributions over time, actively participate in code reviews, and be nominated by an existing Maintainer.
- **Maintainer**: Demonstrate deep understanding of the codebase, a sustained track record as a Committer, and be approved by a majority of existing Maintainers.

Nominations are discussed in the project's community channels. Please reach out to the project team at [g-infra-bada@360.cn](mailto:g-infra-bada@360.cn) if you are interested.

## Contact

For governance-related inquiries, please open an issue or contact [g-infra-bada@360.cn](mailto:g-infra-bada@360.cn).
