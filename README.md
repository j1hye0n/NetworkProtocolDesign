# NetworkProtocolDesign
이동통신 단말 등록 프로토콜


## <개요>

  이동통신 단말 등록 프로토콜은 단말기의 위치가 변함에 따라 가장 가까운 위치의 기지국을 신호 세기를 기준으로, 실시간으로 탐색, 연결하여 등록하는 것을 목표로 한다. 기지국과 단말기 간 텍스트 기반의 데이터를 주고받으며 위치 정보를 업데이하는 프로토콜이다.
 해당 프로토콜은 숙명여자대학교 네트워크프로토콜설계 강의에서 제공한 Cpp코드를 기반으로 수정 보완된 프로토콜이며, 본 specification에서 작성된 If-else 외 다른 이벤트는 모두 무시하고 현재 동작을 수행하는 것으로 정의한다.


### <UserEquipment(UE) : 단말기>

  동작을 시작하면 단말기로 들어오는 기지국 신호의 세기를 분석하여 가장 가까운 기지국을 찾는 것을 첫 번째 목표로 한다. 가까운 기지국을 선정한 뒤 해당 기지국과 연결을 위해 3-way handshake 방법을 사용한다. 한 기지국과 연결에 성공하더라도 다른 기지국과의 거리를 계산하는 동작(신호 세기 비교)이 수행해 가까운 기지국이 업데이트되었을 때 해당 기지국과 연결을 시도하는 것을 최종 목표로 한다.


### <BaseStation : 기지국>

  동작을 시작하면서 임의의 단말기로 기지국 정보가 담긴 텍스트 기반 데이터를 전송하는 것을 목표로 동작한다. 단말기가 해당 기지국에 등록을 시도할 때 단말기에 신호를 보내주는 과정을 수행한다. 단말기 연결 여부와 관련 없이 기지국에 대한 데이터 송신을 근본적인 목표로 한다.

# FSM Design
![Image](https://github.com/user-attachments/assets/d508cb94-2c0a-4080-8dc2-e85fc607aa12)

![Image](https://github.com/user-attachments/assets/3148bace-4b02-49e6-8b33-6fd7fa68fb06)


# Specification
https://docs.google.com/document/d/1CPrHq24S4n9PZwCJH4p2SVPPtfjFayoqZT9yWprQpQs/edit?usp=sharing
