# ClinicManagerQt

Qt SQL 与 Model View 实验项目。程序使用 SQLite 保存用户、患者、医生、科室和操作记录，提供登录检测、患者查询、新增、修改、删除、删除确认和无结果状态提示。

## 构建

```bash
mkdir build && cd build
qmake6 ../ClinicManager.pro
make -j4
```

## 运行

默认账号为 `admin`，密码为 `admin123`。

```bash
./ClinicManager
```

## 自测

```bash
QT_QPA_PLATFORM=offscreen ./ClinicManager --self-test ./clinic-selftest.db
```

自测覆盖数据库连接失败、错误与正确登录、缺失字段、重复身份证、修改后刷新、查询无结果和删除后一致性。
