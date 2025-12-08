cd react-web
npx vite build
cd ..
python clasptree.py ./react-web/dist -o ./include/httpd_content.h -p httpd_ -E ./include/httpd_epilogue.h -s context -b httpd_send_block -e httpd_send_expr -H extended
@echo ClASP Generated