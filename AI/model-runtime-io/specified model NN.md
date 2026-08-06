### 1. kws Summary
① 新 Feature Tensor 与旧 Encoder Cache 一起进入 Encoder
② Encoder 输出若干新的声学时间帧，并更新声音历史 Cache
③ 对每个声学时间帧：
   a. 取上轮最多4条完整 Token 候选路径
   b. Decoder 分别读取每条路径末尾的 Token 上下文
   c. Joiner 把当前声音与每条 Token 历史组合
   d. 得到每条路径接整个词表中每个 Token 的分数
   e. 形成最多 4×V 个新候选
   f. 选择最好的4条
   g. 非Blank路径推进关键词匹配图并加入Boost分数
   h. 保存4条路径，供下一个声学时间帧和下一个Tensor继续
④ 某路径完整匹配关键词、声学概率过阈值且尾随Blank足够
⑤ 输出关键词、Token与时间戳