#include "vega/dicom/data_set.h"
#include "vega/dicom/data_element.h"
#include "vega/dictionary/page.h"
#include "vega/dictionary/private_owner_blocks.h"

namespace vega {
  namespace dicom {
    DataSet::DataSet(std::shared_ptr<DataElement> parent)
      : m_parent(parent),
        m_elements(),
        m_private_owner_blocks(std::make_shared<dictionary::PrivateOwnerBlocks>())
    {}

    const DataSet::length_type DataSet::UNDEFINED_LENGTH = 0xFFFFFFFF;

    DataSet::iterator::iterator(DataSet::iterator::iterator_type it)
      : m_it(it)
    {}

    DataSet::iterator::self_type& DataSet::iterator::operator++() {
      ++m_it;
      return *this;
    }

    std::shared_ptr<DataElement>& DataSet::iterator::operator*() {
      return m_it->second;
    }

    bool DataSet::iterator::operator==(const DataSet::iterator::self_type& rhs) const {
      return m_it == rhs.m_it;
    }

    bool DataSet::iterator::operator!=(const DataSet::iterator::self_type& rhs) const {
      return m_it != rhs.m_it;
    }

    DataSet::const_iterator::const_iterator(DataSet::const_iterator::iterator_type it)
      : m_it(it)
    {}

    DataSet::const_iterator::self_type& DataSet::const_iterator::operator++() {
      ++m_it;
      return *this;
    }

    std::shared_ptr<const DataElement> DataSet::const_iterator::operator*() {
      return std::static_pointer_cast<const DataElement>(m_it->second);
    }

    bool DataSet::const_iterator::operator==(const DataSet::const_iterator::self_type& rhs) const {
      return m_it == rhs.m_it;
    }

    bool DataSet::const_iterator::operator!=(const DataSet::const_iterator::self_type& rhs) const {
      return m_it != rhs.m_it;
    }

    const std::weak_ptr<DataElement>& DataSet::parent() const { return m_parent; }
    std::weak_ptr<DataElement>& DataSet::parent() { return m_parent; }

    void DataSet::add_data_element(std::shared_ptr<DataElement> data_element) {
      if (this->data_element(data_element->tag())) throw vega::Exception("DataSet::add_data_element() -- Cannot add new element as it already exists");
      this->add_private_owner_block_if_relevant(data_element);
      this->m_elements.emplace(data_element->tag(), data_element);
    }

    std::shared_ptr<const dictionary::Page> DataSet::page_for(const Tag& tag) const {
      // Try to find in private owners first
      if (tag.is_private()) {
        auto block = m_private_owner_blocks->find_block(tag);
        if (block) {
          auto private_owner = block->private_owner();
          auto page = private_owner->find_page(tag);
          if (page) return page;
        }
      }

      return vega::dictionary::Dictionary::instance().page_for(tag);
    }

    size_t DataSet::size() const { return m_elements.size(); }

    std::shared_ptr<DataElement> DataSet::data_element(const std::string& name) {
      auto page = vega::dictionary::Dictionary::instance().page_for(name);
      if (!page) return nullptr;

      return this->data_element(page->tag_mask());
    }

    std::shared_ptr<DataElement> DataSet::data_element(const TagMask& tag_mask) {
      if (tag_mask.is_single()) {
        return this->data_element(tag_mask.singular_tag());
      }

      for (auto data_element : *this) {
        if (tag_mask.contains(data_element->tag())) return data_element;
      }

      return nullptr;
    }

    std::shared_ptr<DataElement> DataSet::data_element(const Tag& tag) {
      auto it = m_elements.find(tag);
      if (it == m_elements.end()) return nullptr;
      return it->second;
    }

    std::shared_ptr<const DataElement> DataSet::data_element(const std::string& name) const {
      auto page = vega::dictionary::Dictionary::instance().page_for(name);
      if (!page) return nullptr;

      if (page->tag_mask().is_single()) {
        return this->data_element(page->tag_mask().singular_tag());
      }

      for (auto data_element : *this) {
        if (page->tag_mask().contains(data_element->tag())) return data_element;
      }

      return nullptr;
    }

    std::shared_ptr<const DataElement> DataSet::data_element(const Tag& tag) const {
      auto it = m_elements.find(tag);
      if (it == m_elements.end()) return nullptr;
      return std::const_pointer_cast<const DataElement>(it->second);
    }

    bool DataSet::is_undefined_length() const { return this->length == UNDEFINED_LENGTH; }

    bool DataSet::operator==(const DataSet& other) const {
      if (this->size() != other.size()) return false;

      for (const auto& it : *this) {
        const Tag& tag = it->tag();
        auto other_data_element = other.data_element(tag);

        if (!other_data_element) return false;
        if (*it != *other_data_element) return false;
      }

      return true;
    }

    bool DataSet::operator!=(const DataSet& other) const {
      return !(*this == other);
    }

    void DataSet::log(Formatter& formatter) const {
      for (auto data_element : m_elements) {
        data_element.second->log(formatter);
      }
    }

    void DataSet::add_private_owner_block_if_relevant(std::shared_ptr<DataElement> data_element) {
      if (data_element->is_sequence()) return;
      if (!data_element->tag().is_private_owner()) return;

      std::string name = (*data_element->get_manipulator<manipulators::LongStringManipulator>())[0];

      auto private_owner = vega::dictionary::Dictionary::instance().private_owner(name);
      if (!private_owner) return;

      // Here it is a private owner element
      m_private_owner_blocks->add_private_owner_block(std::make_shared<dictionary::PrivateOwnerBlock>(
        private_owner,
        data_element->tag().group(),
        data_element->tag().element()
      ));
    }
  }
}
