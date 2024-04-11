#!/usr/bin/env python
import cgi
import os

# 지정된 디렉토리에 파일 저장하는 함수
def save_uploaded_file(fileitem, upload_dir):
    if not os.path.exists(upload_dir):
        os.makedirs(upload_dir)

    filepath = os.path.join(upload_dir, os.path.basename(fileitem.filename))

    with open(filepath, 'wb') as f:
        while True:
            chunk = fileitem.file.read(1024)
            if not chunk:
                break
            f.write(chunk)
    return filepath

# HTML response 생성하는 함수
def generate_html_response(message):
    print("Content-type: text/html\n")
    print(f"<html><body><h1>{message}</h1></body></html>")

# 메인 CGI 처리
if __name__ == "__main__":
    form = cgi.FieldStorage()

    # 업로드할 파일이 있는지 확인
    if "fileToUpload" not in form:
        generate_html_response("파일을 선택해주세요.")
    else:
        fileitem = form["fileToUpload"]

        # 파일을 지정된 디렉토리에 저장
        upload_dir = "/upload/"
        saved_filepath = save_uploaded_file(fileitem, upload_dir)

        generate_html_response(f"파일 업로드 완료: {saved_filepath}")
