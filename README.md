# HTTP Server
## About
C++98으로 작성된 HTTP 1.1 서버입니다.

## Usage
``` bash
make
http-server [configuration file] 	# conf/default.conf
```
## Configuration File

``` bash
server {
	listen 8080; 	# 서버의 [host:]port
	server_name localhost; 	# 서버 명
	error_page error_page.html; 	# 기본 오류 페이지
	client_max_body_size 1000000; 	# 최대 HTTP Request Body 크기
	auto_index off; 	# 디렉토리 리스팅 사용 여부 설정
	index index.html; 	# index 페이지

	location / { 	# URI 라우팅 규칙
		allowed_method GET POST DELETE; 	# 허용된 메서드
		root /www; 	# 루트 디렉토리
		auto_index off;
		index index.html;
		upload_store upload/; 	# 파일 업로드 경로
	}
}
```
# Introduce

HTTP (Hypertext Transfer Protocol)는 인터넷을 통해 정보를 송수신하는 프로토콜입니다.

HTTP 웹 서버는 브라우저 같은 클라이언트로부터 HTTP 요청을 수신하고 응답하는 소프트웨어 애플리케이션입니다. 

HTTP는 요청과 응답으로 구성됩니다. 클라이언트가 서버에서 웹페이지를 검색하려고 할 때, HTTP 요청을 서버에 보냅니다. 서버는 요청을 처리한 후 HTTP 응답을 다시 보냅니다.

### HTTP 메시지 형식

```
start-line CRLF
Headers CRLF
CRLF(end of headers)
[message-body]
```
### HTTP Request

요청 라인은 세 부분으로 구성됩니다: 메서드, 경로, 그리고 HTTP 버전. 메서드는 클라이언트가 수행하고자 하는 작업을 지정하며, 예를 들어 GET 또는 POST 등이 있습니다. 경로 또는 URI는 서버에서 자원의 위치를 지정합니다. HTTP 버전은 사용 중인 HTTP 프로토콜의 버전을 나타냅니다.

헤더는 요청에 대한 추가 정보를 포함합니다. 예를 들어 서버의 호스트 이름 및 사용 중인 브라우저의 유형이 있습니다.

|Method|Description|Possible Body|
|:----|----|:----:|
|**GET** | 특정 자원이나 자원 집합을 검색하며, 데이터/자원에 영향을 주지 않음| No|
|**POST** | 요청 콘텐츠에 대해 자원별 처리를 수행| Yes|
|**DELETE** | URI로 지정된 대상 자원을 삭제| Yes|

*이 HTTP 서버는 GET, POST, DELETE 메서드를 지원하며, Query를 비롯한 모든 URI 표현을 지원합니다. 또, 조건부 요청, 본문 메타데이터, Chunked-Encoding에 대한 HTTP	 헤더를 해석할 수 있습니다.*


```
GET /index.html HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
```
### HTTP Response

상태 라인은 세 부분으로 구성됩니다: HTTP 버전, 상태 코드, 그리고 이유 구문. 상태 코드는 요청의 결과를 나타내며, 예를 들어 200 OK 또는 404 Not Found 등이 있습니다. 이유 구문은 상태 코드에 대한 간단한 설명입니다. 상태 코드의 간략한 요약은 다음과 같습니다

|Method|Description||
|:----|----|:----:|
|**1xx** | 정보 메시지만을 나타냅니다|
|**2xx** | 어떤 형태의 성공을 나타냅니다|
|**3xx** | 클라이언트를 다른 URL로 리다이렉트합니다|
|**4xx** | 클라이언트 에러를 나타냅니다|
|**5xx** | 서버 에러를 나타냅니다|


*이 HTTP 서버는 리다이렉션을 지원하며, RFC 9110, 9112에 기술된 대로 모든 HTTP 응답 상태코드를 정확히 반환합니다.*
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234

<Message Body>
```
## Server Core
웹 서버의 네트워킹 부분으로, TCP 연결을 처리하고 들어오는 요청을 수신하고 응답을 보내는 작업을 수행합니다. 이는 웹 서버의 저수준 네트워킹 작업을 책임지며, 소켓 생성 및 관리, 입력 및 출력 스트림 처리, 서버와 클라이언트 간의 데이터 흐름 관리를 포함합니다.

*이 HTTP 서버는 Non-Blocking I/O 방식으로 다중 요청을 처리하고 지원합니다.*
## Request Parser
Transaction 클래스의 ParseRequestHeader(), ParseRequestBody() 메서드가 HTTP Request를 파싱하고, Http 클래스와 Transaction, Uri 클래스에 각각 데이터를 저장합니다.

파싱 규칙과 자료구조는 RFC 9110 - HTTP Semantics와 RFC 9112 - HTTP 1.1를 기준으로 작성되었습니다.

## Response Builder
Transaction 클래스의 HttpProcess() 메서드를 통해 요청에 따른 작업을 수행합니다. 이어서 CreateResponseMessage() 메서드를 통해 작업 결과에 따른 HTTP 응답을 생성합니다.

HTTP 상태코드와 응답 메시지 생성 규칙은 RFC 9110 - HTTP Semantics와 RFC 9112 - HTTP 1.1를 기준으로 작성되었습니다.

## CGI

CGI는 웹 서버에서 외부 프로그램을 실행하기 위한 표준입니다. CGI 프로그램은 Perl, Python, bash 등 어떤 프로그래밍 언어로도 작성할 수 있는 간단한 스크립트로, 사용자가 CGI 프로그램이 처리해야 하는 웹 페이지를 요청하면, 웹 서버는 해당 프로그램을 실행하고 그 결과를 사용자의 웹 브라우저에 반환합니다.

*이 HTTP 서버는 CGI 스크립트를 지원하며, 기본적으로 파일 업로드를 기능을 지원하기 CGI 스크립트를 포함하고 있습니다.*
