#!/usr/bin/perl -w
  use CGI;

  $upload_dir = "../upload";

  $query = new CGI;

  $filename = $query->param("fileToUpload");
  $filename =~ s/.*[\/\\](.*)/$1/;
  $upload_filehandle = $query->upload("photo");

  open UPLOADFILE, ">$upload_dir/$filename";

  while ( <$upload_filehandle> )
  {
    print UPLOADFILE;
  }

  close UPLOADFILE;

  print $query->header ( );
  print <<END_HTML;

	<!DOCTYPE html>
	<html lang="en">
	<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>Thanks</title>
	</head>
	<body>
	<P>Thanks for uploading!</P>
	</body>
	</html>

  END_HTML
