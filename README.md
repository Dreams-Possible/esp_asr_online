# esp_asr_online
一个简单的ESP在线语音识别历程。
使用FunASR开源项目的在线语音识别测试API。特别感谢这个开源语音识别项目。
使用了esp_websocket_client库提供websocket的API支持，受限于上传大小，同样需要自行添加它。
但由于某些未知原因，数据流传输速度过慢，这使得理论上的实时识别延迟巨大，几乎完全丧失了实时的可用性，因此这个项目大概率止步于此，也不会提供类似esp_llm那样灵活的回调方式，仅仅作为Demo参考使用。
![image](https://github.com/user-attachments/assets/6d5950bd-dc78-4fca-b227-7da01064dc16)
