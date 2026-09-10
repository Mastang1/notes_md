### 理解
> **场景本质： _给一个系统不同的 Event作为输入，该系统根据自己的State，做出不同的Actions, 并执行状态的跃迁（State Transition）；

```c
/*
实际用过的场景：
	1. 最简单的通信协议帧解析处理过程；亦即：State有多种，但是input的Event就一种：就是Byte arrived；
	2. User通过按钮或者其他输入接口发送不同的command给系统的过程：亦即：State有多种，input也有多种；
*/

```

> **实现-1： _switch(Event), case event1: 处理不同states的state next及action 处理; ... case eventx: ...;
> 		（导致逻辑复杂不容易维护，每个event都要处理不同state的动作）_

> **实现-2： _思路是将 “实现-1"中的不同的 ”event-state-action-next_state“ 多对多组合，由分支改为静态的结构体数组。
> 			形如：ELEMENT:{event-x, state-n, action-m, next_state-l},由此构成数组TABLE；
> 			RUN：Event为入参，基于【state,event】对查询数组TABLE，调用action(统一的函数签名，指针调用)，
> 				更新Global_state为next_state-l;
> 				