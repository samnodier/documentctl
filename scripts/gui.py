import sys
import re
import os
import resources_rc # type: ignore
import fitz
os.environ["QT_WAYLAND_DISABLE_WINDOWDECORATION"] = "0"
from PyQt6.QtWidgets import (QApplication, QFileDialog, QHBoxLayout, QMainWindow, QMessageBox, QPushButton, QSizePolicy, QSlider, QWidget,
                             QTextEdit, QScrollArea, QListWidgetItem,
                             QVBoxLayout, QLineEdit, QLabel,
                             QSplitter, QListWidget, QStatusBar,
                             QToolBar, QSpinBox
                             )

from PyQt6.QtGui import QAction, QIcon, QShortcut, QKeySequence, QFont, QImage, QPixmap
from PyQt6.QtCore import QSize, QThread, Qt, pyqtSignal

# Import the engine wrapper

from search_engine import SearchEngine

class EngineSearchWorker(QThread):
    results_ready = pyqtSignal(list) # Sends the object

    def __init__(self, engine, query: str):
        super().__init__()
        self.engine = engine
        self.query = query

    def run(self):
        results = self.engine.search(self.query)
        #  Prefetch snippets while still in the background
        for res in results:
            res.snippet = self.engine.get_snippet(res) or "No snippet available"

        # Send results back to the GUI
        self.results_ready.emit(results)

class LeftPanel(QWidget):
    """
    Collapsible left panel containing:
     - Search input + button
     - Results list (clicking a result jumps to that page)
    """
    result_selected = pyqtSignal(object)

    def __init__(self, parent=None):
        super().__init__(parent)
        # 1. Setup the Engine
        self.engine = SearchEngine()

        if self.engine.load():
            self.status_msg = "Index loaded successfully"
        else:
            self.status_msg = "No index found. Please index a directory"

        self.setMinimumWidth(220)
        self.setMaximumWidth(400)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        # Search Header label
        header = QLabel()
        header.setText(
            '<img src=":/icons/search.png" width="16" height="16" style="vertical-align: middle;"> '
            'Search'
        )
        header.setFont(QFont("Satoshi", 10, QFont.Weight.Medium))
        layout.addWidget(header)

        # Search input row
        search_row = QHBoxLayout()
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("Search in PDF...")
        self.search_input.returnPressed.connect(self.on_search_clicked)
        search_row.addWidget(self.search_input)

        self.search_btn = QPushButton("Go")
        self.search_btn.setFixedWidth(36)
        self.search_btn.clicked.connect(self.on_search_clicked)
        search_row.addWidget(self.search_btn)
        layout.addLayout(search_row)

        # eesults count label
        self.result_count_label = QLabel("No results")
        self.result_count_label.setStyleSheet("color: grey; font-size: 11px;")
        layout.addWidget(self.result_count_label)

        self.results_list = QListWidget()
        self.results_list.setWordWrap(True)
        # Single line also jumps
        self.results_list.itemClicked.connect(self._on_result_clicked)
        layout.addWidget(self.results_list, stretch = 1)

        self._doc = None
        self._search_worker = None

    def set_document(self, doc:fitz.Document):
        """Call this when a new PDF is opened."""
        self._doc = doc

    def _on_result_clicked(self, item: QListWidgetItem):
        # Retrie the SearchResult object we attached earlier
        res = item.data(Qt.ItemDataRole.UserRole)

        if res is not None:
            self.result_selected.emit(res)

    def on_search_clicked(self):
        query = self.search_input.text().strip()
        if not query:
            return

        self.search_btn.setEnabled(False)
        self.search_btn.setText("...")
        self.results_list.clear()
        self.result_count_label.setText("Searching...")

        # Run search in the background
        self._search_worker = EngineSearchWorker(self.engine, query)
        self._search_worker.results_ready.connect(self._populate_results)
        self._search_worker.finished.connect(self._search_done)
        self._search_worker.start()


    def _populate_results(self, results :list):
        self.results_list.clear()
        query = self.search_input.text().strip()

        # Display the "Rich Data" from C Metadata structs
        for res in results:
            # Grab the snippet
            snippet = getattr(res, 'snippet',  "No snippet available")

            query = self.search_input.text().strip()

            if query.lower() in snippet.lower():
                highlighted_snippet = re.sub(f"({query})", r"**\1**", snippet, flags=re.IGNORECASE)
            else:
                highlighted_snippet = snippet

            item = QListWidgetItem()

            # Format the text: Title and Metadata on top, Snippet below
            display_text = (
                f"{res.title or 'Unknown'}\n"
                f"Page: {res.page_num + 1} | Author: {res.author or 'Unknown'}\n"
                f"\"{highlighted_snippet}\""
            )
            item.setText(display_text)

            # 4. Attach the full result object to the Hidden Pocket
            item.setData(Qt.ItemDataRole.UserRole, res)

            # 5. Styling. Make the snippet look distinct
            item.setToolTip(f"Full snippet: {snippet}")

            self.results_list.addItem(item)

        count = len(results)
        self.result_count_label.setText(
            f"{count} result{'s' if count != 1 else '' } found"
            if count else "No results found"
        )

    def _search_done(self):
        self.search_btn.setEnabled(True)
        self.search_btn.setText("Go")

# Right Panel - PDF Viewer
class PDFViewer(QScrollArea):
    """
    Renders a single PDF page using PyMuPDF and displays it inside a scroll area
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setWidgetResizable(True)

        # Inner label that holds the page pixmap
        self._page_label = QLabel("Open a PDF to get started")
        self._page_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._page_label.setStyleSheet("color: #888; font-size: 16px;")
        self._page_label.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding
        )
        self.setWidget(self._page_label)

        self._doc = None
        self._zoom = 1.5
        self._current_page = 0
        self._highlight_term = None

    def set_document(self, doc: fitz.Document):
        self._doc = doc
        self._current_page = 0
        self._highlight_term = None
        self.render_page()

    def set_zoom(self, zoom: float):
        self._zoom = zoom
        self.render_page()

    def render_page(self, highlight_term = None):
        if self._doc is None:
            return

        if highlight_term:
            self._highlight_term = highlight_term

        page = self._doc[self._current_page]

        # If we have a highlight term apply it to the page temporarily
        if self._highlight_term:
            # Search for intances of the word
            text_instances = page.search_for(self._highlight_term)
            for inst in text_instances:
                # Add a highlight annotation (Yellow)
                annot = page.add_highlight_annot(inst)
                annot.update()

        matrix = fitz.Matrix(self._zoom, self._zoom)
        # alpha=False ensures a white background for PDFs
        pix = page.get_pixmap(matrix = matrix, alpha = False)

        # Convert fitz.Pixmap -> QPixmap
        img = QImage(
            pix.samples,
            pix.width,
            pix.height,
            pix.stride,
            QImage.Format.Format_RGB888
        )
        self._page_label.setPixmap(QPixmap.fromImage(img))
        self._page_label.adjustSize()

        # Clean up highlight so it doesn't stay on the PDF performanently
        if self._highlight_term:
            for annot in page.annots():
                page.delete_annot(annot)

    def goto_page(self, page_num: int):
        if self._doc is None:
            return
        page_num = max(0, min(page_num, len(self._doc) - 1))

        # if the user manually navigates, clear the previous search highlight
        # If they came from a search result, handle_search_result_selection will set a new one
        if self._current_page != page_num:
            self._highlight_term = None

        self._current_page = page_num
        self.render_page()
        # Scroll back to the top of the new page
        v_bar = self.verticalScrollBar()
        if v_bar:
            v_bar.setValue(0)

    def wheelEvent(self, a0):
        if a0 is None:
            return
        if a0.modifiers() == Qt.KeyboardModifier.ControlModifier:
            # Calculate the zoom direction (delta is between 120 and -120)
            angle = a0.angleDelta().y()
            zoom_step = 0.1

            if angle > 0:
                self._zoom += zoom_step
            else:
                self._zoom = max(0.2, self._zoom - zoom_step)
            self.render_page()

        else:
            super().wheelEvent(a0)

    @property
    def current_page(self) -> int:
        return self._current_page

    @property
    def page_count(self) -> int:
        return len(self._doc) if self._doc else 0

class ContextView(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.setSpacing(2)

        # Mini search bar for the text panel
        self.search_bar = QLineEdit()
        self.search_bar.setPlaceholderText("Find in text...")
        self.search_bar.setStyleSheet("background: #313244; font-size: 11px; height: 20px;")
        self.search_bar.textChanged.connect(self.find_text)
        layout.addWidget(self.search_bar)

        # the text area
        self.text_area = QTextEdit()
        self.text_area.setReadOnly(True)
        self.text_area.setPlaceholderText("Raw text will appear here...")
        self.text_area.setStyleSheet("""
            QTextEdit {
                background: #181825;
                color: #a6adc8;
                border-left: none;
                font-family: 'Satoshi', sans-serif;
                font-size: 13px;
            }
        """)
        layout.addWidget(self.text_area)

    def setPlainText(self, text: str):
        self.text_area.setPlainText(text)

    def find_text(self, text: str):
        """Simple internal search for the context panel"""
        if not text:
            return

        # Standard QTextEdit find logic
        self.text_area.find(text)


class PDFReaderWindow(QMainWindow):
    """
    Main application Window

    Layout:
        QMainWindow
        └── CentralWidget
            └── QHBoxLayout
                └── QSplitter (horizontal)
                    ├── Left Panel (collapsible)
                    └── PDFViewer (resizable scroll area)

    """

    def __init__(self):
        super().__init__()


        # 2. Window Properties
        self.setWindowTitle("Document Intelligence Toolkit")
        self.resize(1200, 800)
        self._doc = None

        self._build_toolbar()
        self._build_central()
        self._build_statusbar()

                # Apply a clean base stylesheet
        self.setStyleSheet("""
            QMainWindow { background: #1e1e2e; }
            QToolBar    { background: #2a2a3e; border-bottom: 1px solid #3a3a5a; spacing: 4px; }
            QPushButton { background: #3a3a5a; color: #cdd6f4; border: none;
                          padding: 4px 10px; border-radius: 4px; }
            QPushButton:hover    { background: #585b70; }
            QPushButton:disabled { background: #2a2a3e; color: #555; }
            QLineEdit   { background: #313244; color: #cdd6f4; border: 1px solid #45475a;
                          border-radius: 4px; padding: 4px; }
            QListWidget { background: #1e1e2e; color: #cdd6f4; border: none; }
            QListWidget::item { padding: 10px; border-bottom: 1px solid #2a2a3e; color: #cdd6f4; }
            QListWidget::item:hover { background: #313244; }
            QListWidget::item:selected { background: #3a3a5a; }
            QScrollArea { background: #181825; border: none; }
            QLabel      { color: #cdd6f4; }
            QSplitter::handle { background: #3a3a5a; width: 3px; }
            QStatusBar  { background: #2a2a3e; color: #888; }
        """)
        # 4. Emergency Quit (Hyprland)
        self.quit_shortcut = QShortcut(QKeySequence("Ctrl+W"), self)
        self.quit_shortcut.activated.connect(self.close)


    def _build_toolbar(self):
        tb = QToolBar("MainToolbar")
        tb.setMovable(False)
        tb.setIconSize(QSize(16, 16))
        self.addToolBar(tb)

        # Open file
        self.act_open = QAction(QIcon(":/icons/file_open.png"), "Open", self)
        self.act_open.setShortcut("Ctrl+O")
        self.act_open.setToolTip("Open PDF file (Ctrl+O)")
        self.act_open.triggered.connect(self.open_pdf)
        tb.addAction(self.act_open)

        tb.addSeparator()

        # Toggle sidebar
        self.act_toggle_sidebar = QAction(QIcon(":icons/dock_to_right.png"), "Hide Panel", self)
        self.act_toggle_sidebar.setShortcut("Ctrl+B")
        self.act_toggle_sidebar.setToolTip("Toggle search panel (Ctrl+B)")
        self.act_toggle_sidebar.triggered.connect(self.toggle_sidebar)
        tb.addAction(self.act_toggle_sidebar)

        tb.addSeparator()

        # Toggle context view
        self.act_toggle_context = QAction(QIcon(":icons/dock_to_left.png"), "Show Text", self)
        self.act_toggle_context.setShortcut("Ctrl+I")
        self.act_toggle_context.setToolTip("Toggle Raw Text Panel (Ctrl+I)")
        self.act_toggle_context.triggered.connect(self.toggle_context_view)
        tb.addAction(self.act_toggle_context)

        # Prev / Next Page
        self.act_prev = QAction(QIcon(":/icons/arrow_back.png"), "Prev", self)
        self.act_prev.setShortcut("Left")
        self.act_prev.setEnabled(False)
        self.act_prev.triggered.connect(self.prev_page)
        tb.addAction(self.act_prev)

        self.act_next = QAction(QIcon(":/icons/arrow_forward.png"), "Prev", self)
        self.act_next.setShortcut("Right")
        self.act_next.setEnabled(False)
        self.act_next.triggered.connect(self.next_page)
        tb.addAction(self.act_next)

        tb.addSeparator()

        tb.addWidget(QLabel("  Page "))
        self.page_spin = QSpinBox()
        self.page_spin.setMinimum(1)
        self.page_spin.setMaximum(1)
        self.page_spin.setFixedWidth(60)
        self.page_spin.setEnabled(False)
        self.page_spin.valueChanged.connect(self._on_page_spin_changed)
        tb.addWidget(self.page_spin)

        self.page_total_label = QLabel(" / — ")
        tb.addWidget(self.page_total_label)

        tb.addSeparator()
        tb.addWidget(QLabel(" Zoom "))
        zoom_slider = QSlider(Qt.Orientation.Horizontal)
        zoom_slider.setRange(50, 300); zoom_slider.setValue(150)
        zoom_slider.valueChanged.connect(lambda v: self._pdf_viewer.set_zoom(v / 100))
        tb.addWidget(zoom_slider)

    def _build_central(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QHBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # QSplitter holds left panel + pdf viewer
        # User can drag to resize
        self._splitter = QSplitter(Qt.Orientation.Horizontal)
        self._splitter.setHandleWidth(4)

        # LEFT Panel: Search & Results
        self._left_panel = LeftPanel()
        self._left_panel.result_selected.connect(self.handle_search_result_selection)
        self._splitter.addWidget(self._left_panel)

        # RIGHT Panel: PDF Viewer
        self._pdf_viewer = PDFViewer()
        self._splitter.addWidget(self._pdf_viewer)

        v_bar = self._pdf_viewer.verticalScrollBar()
        if v_bar is not None:
            v_bar.valueChanged.connect(self._handle_continuous_scroll)
        # CONTEXT VIEW
        self._context_view = ContextView()
        self._splitter.addWidget(self._context_view)

        # Set initial sizes
        self._splitter.setSizes([260, 940, 0])
        # Prevent the viewer from collapsing to zero
        self._splitter.setCollapsible(0, True)
        self._splitter.setCollapsible(1, False)
        self._splitter.setCollapsible(2, True)

        layout.addWidget(self._splitter)

    def _build_statusbar(self):
        self.status = QStatusBar()
        self.setStatusBar(self.status)
        self.status.showMessage("Ready — open a PDF or Search")

    def open_pdf(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Open PDF", "", "Open Files (*.pdf);;All Files (*)"
        )
        if not path:
            return

        success = self.load_document(path)
        if success:
            self.status.showMessage(f"Successfully loaded: {os.path.basename(path)}")
            self.goto_page(0)

    def handle_search_result_selection(self, res):
        """Triggered by clicking a search result in the left pane"""
        query = self._left_panel.search_input.text().strip()

        if self.load_document(res.doc_path):
            self._pdf_viewer._highlight_term = query
            self.goto_page(res.page_num)
            self.status.showMessage(f"Found '{query}' on page{res.page_num + 1}")

    def load_document(self, path):
        """"Load PDF and sync UI components"""
        try:
            abs_path = os.path.abspath(path)
            if self._doc and self._doc.name:
                current_path = os.path.abspath(self._doc.name)
                if current_path == abs_path:
                    return True

            if self._doc:
                self._doc.close()

            self._doc = fitz.open(abs_path)

            self._left_panel.set_document(self._doc)
            self._pdf_viewer.set_document(self._doc)

            page_count = len(self._doc)
            self.page_spin.blockSignals(True)
            self.page_spin.setMaximum(page_count)
            self.page_spin.setValue(1)
            self.page_spin.blockSignals(False)
            self.page_spin.setEnabled(True)

            self.page_total_label.setText(f"  / {page_count}  ")
            self.act_prev.setEnabled(True)
            self.act_next.setEnabled(True)
            self.act_toggle_sidebar.setText(
                '<img src=":/icons/dock_to_right.png" width="16" height="16" style="vertical-align: middle;"> '
                'Hide Panel'
            )

            self.setWindowTitle(f"Toolkit - {os.path.basename(abs_path)}")

            return True
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not open PDF: {e}")

    # Navigation
    def prev_page(self):
        if self._doc and self._pdf_viewer.current_page > 0:
            self.goto_page(self._pdf_viewer.current_page - 1)

    def next_page(self):
        if self._doc and self._pdf_viewer.current_page < len(self._doc) - 1:
            self.goto_page(self._pdf_viewer.current_page + 1)

    def goto_page(self, page_num:int):
        """Central method to change page - keep toolbar and viewer in sync"""
        if not self._doc:
            return

        # Bound checking
        page_count = len(self._doc)
        page_num = max(0, min(page_num, page_count - 1))

        # Update the viewer
        self._pdf_viewer.goto_page(page_num)

        # Extract and display raw text in the context sidebar
        try:
            page = self._doc[page_num]
            raw_text = str(page.get_text("text"))
            self._context_view.setPlainText(raw_text)
        except Exception as e:
            print(f"Text extraction failed: {e}")

        # Sync Toolbar (Block spin signal to avoid feedback loop)
        self.page_spin.blockSignals(True)
        self.page_spin.setValue(page_num + 1)
        self.page_spin.blockSignals(False)

        # Update nav buttons states
        self.act_prev.setEnabled(page_num > 0)
        self.act_next.setEnabled(page_num < len(self._doc) - 1)
        self.status.showMessage(f"Page {page_num + 1} of {len(self._doc)}")

    def _on_page_spin_changed(self, value: int):
        """ Called when user types a pgae number in spin box"""
        if self._doc:
            self.goto_page(value-1)

    def toggle_sidebar(self):
        """
        Collapse the left panel by settings its width to 0
        or restore to 260px
        """
        sizes = self._splitter.sizes()
        if sizes[0] > 0:
            # Collapse
            self._splitter.setSizes([0, sum(sizes)])
            self.act_toggle_sidebar.setText(
                '<img src=":/icons/dock_to_right.png" width="16" height="16" style="vertical-align: middle;"> '
                'Show Panel'
                )
        else:
            # Expand
            total = sum(sizes)
            self._splitter.setSizes([260, total - 260])
            self.act_toggle_sidebar.setText(
                '<img src=":/icons/dock_to_right.png" width="16" height="16" style="vertical-align: middle;"> '
                'Hide Panel'
                )

    def toggle_context_view(self):
        """Toggle the rigth side ContextView panel and update icon/Text"""
        sizes = self._splitter.sizes()
        if sizes[2] > 0:
            # Collapse
            new_viewer_width = sizes[1] + sizes[2]
            self._splitter.setSizes([sizes[0], new_viewer_width, 0])
            self.act_toggle_context.setText("Show Text")
            self.status.showMessage("Context Panel Hidden")
        else:
            total_available = sizes[1]
            right_width = 350
            center_width = max(200, total_available - right_width)

            self._splitter.setSizes([sizes[0], center_width, right_width])
            self.act_toggle_context.setText("Hide Text")
            self.status.showMessage("Context Panel Visible")

    def _handle_continuous_scroll(self, value):
        if not self._doc:
            return

        v_bar = self._pdf_viewer.verticalScrollBar()
        if v_bar is None:
            return

        # Check if we hit the bottom
        if value == v_bar.maximum() and v_bar.maximum() > 0:
            current = self._pdf_viewer.current_page
            if current < len(self._doc) - 1:
                self.next_page()
                v_bar.setValue(1)
        # If we hit the top
        elif value == 0:
            current = self._pdf_viewer.current_page
            if current > 0:
                self.prev_page()
                # Reset Scroll to the bottom of the previous page
                QApplication.processEvents()
                v_bar.setValue(v_bar.maximum() -1)

    def keyPressEvent(self, a0):
        if a0 is None:
            return

        key = a0.key()
        if key == Qt.Key.Key_Right or key == Qt.Key.Key_PageDown:
            self.next_page()
        elif key == Qt.Key.Key_Left or key == Qt.Key.Key_PageUp:
            self.prev_page()
        else:
            super().keyPressEvent(a0)

    def closeEvent(self, a0):
        if a0 is None:
            return

        if self._doc:
            try:
                self._doc.close()
                self._doc = None
            except Exception as e:
                print(f"Error closing document: {e}")
        a0.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setDesktopFileName("PDF Reader")

    window = PDFReaderWindow()
    window.show()
    sys.exit(app.exec())
