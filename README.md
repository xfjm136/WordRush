# WordRush

`wordrush` 是一个面向 Linux 终端的高强度词汇训练器，目标不是打卡，而是快速掌握重点词。

## 当前词本

- `cet4-core`：CET-4 重点词
- `cet6-core`：CET-6 重点词

## 特性

- 词本按文件管理，支持导入、删除、合并、拆分
- 拼写回忆支持 `中译英 / 英译中 / 混译`
- 终端优先，界面尽量简洁
- 带陪练伙伴，训练时会有随机台词和反馈

## 数据目录

- `data/library/books/`：词本文件
- `data/state/progress.tsv`：进度数据

## 构建

```bash
cmake -S /data/WordRush -B /data/WordRush/build -G Ninja
cmake --build /data/WordRush/build
```

## 运行

```bash
/data/WordRush/build/wordrush
```

