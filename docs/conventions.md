## Git Convention
GitHub-flow 전략을 사용하며 branch 이름은 다음과 같은 규칙을 포함한다.
- feature/#`이슈 번호`-`기능`

## Code Convention
Code Style의 일반적인 규칙들을 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)를 적용한다.

## Commit Convention
### Subject
- feat : 기능 추가
- fix : 버그 수정
- docs : 문서 변경
- refactor : 코드 리펙토링
- chore : 빌드 프로세스, 도구 설정 변경 등 기타 작업

### Body
- 선택사항
- 레포지토리 변경사항 작성

### Footer
- Fixes: issue 진행중
- Resolves: issue 해결 완료
- Ref: 참고할 issue
- Related To: 해당 커밋 관련 issue (해결 필요 issue)

## Pull Request
- 제목: [#`기능 번호`] `변경 사항`
- Issue와의 연동